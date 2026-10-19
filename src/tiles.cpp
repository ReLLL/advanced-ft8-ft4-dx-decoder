// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "tiles.h"
#include <utils/opengl_include_code.h>
#include <curl/curl.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <charconv>
#include <ctime>
#include <cctype>

namespace dx {
namespace {
using Clock=std::chrono::steady_clock;
size_t receive(char* data,size_t size,size_t count,void* context){auto* bytes=static_cast<std::vector<unsigned char>*>(context);auto length=size*count;if(length>1024*1024||bytes->size()+length>1024*1024)return 0;bytes->insert(bytes->end(),data,data+length);return length;}
size_t headers(char* data,size_t size,size_t count,void* context){auto* text=static_cast<std::string*>(context);auto length=size*count;if(text->size()+length>16384)return 0;text->append(data,length);return length;}
}
struct Tiles::Impl {
 struct Entry {std::vector<unsigned char> pixels;unsigned int texture=0;Clock::time_point retry{},used{};};
 std::filesystem::path cache;std::map<TileKey,Entry> entries;std::vector<TileKey> desired;
 std::mutex mutex,diskMutex;std::condition_variable cv;std::vector<std::thread> workers;std::atomic<bool> stopping{false};std::string error;std::once_flag initializeDisk;
 Clock::time_point viewportStart=Clock::now(),viewportDone=viewportStart;
 std::atomic<uint64_t> requests{0},diskHits{0},diskBytes{0};
 explicit Impl(std::filesystem::path path):cache(std::move(path)){for(int i=0;i<4;i++)workers.emplace_back([this]{run();});}
 struct Request {Impl* owner;TileKey key;bool cancelled=false;};
 static int progress(void* context,curl_off_t,curl_off_t,curl_off_t,curl_off_t){auto& request=*static_cast<Request*>(context);std::lock_guard<std::mutex> lock(request.owner->mutex);request.cancelled=request.owner->stopping||std::find(request.owner->desired.begin(),request.owner->desired.end(),request.key)==request.owner->desired.end();return request.cancelled?1:0;}
 std::filesystem::path file(TileKey k){return cache/(k.layer==2?"esri-street":"humanitarian")/std::to_string(k.z)/std::to_string(k.x)/(std::to_string(k.y)+(k.layer==2?".jpg":".png"));}
 std::vector<unsigned char> fetch(TileKey key,CURL* curl){
  std::vector<unsigned char> bytes,stale;auto path=file(key);std::error_code ec;
  if(std::filesystem::is_regular_file(path,ec)&&std::filesystem::file_size(path,ec)<=1024*1024){
   int64_t expires=0;std::ifstream expiry(path.string()+".expiry");expiry>>expires;
   std::ifstream cached(path,std::ios::binary);stale.assign(std::istreambuf_iterator<char>(cached),{});if(expires>std::time(nullptr)){++diskHits;return stale;}
  }
  curl_easy_reset(curl);
#ifdef _WIN32
  // Use the Windows trust store; never depend on the developer's MSYS path.
  curl_easy_setopt(curl,CURLOPT_SSL_OPTIONS,CURLSSLOPT_NATIVE_CA);curl_easy_setopt(curl,CURLOPT_CAINFO,nullptr);curl_easy_setopt(curl,CURLOPT_CAPATH,nullptr);
#endif
  auto url=key.layer==2?"https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/"+std::to_string(key.z)+"/"+std::to_string(key.y)+"/"+std::to_string(key.x):"https://a.tile.openstreetmap.fr/hot/"+std::to_string(key.z)+"/"+std::to_string(key.x)+"/"+std::to_string(key.y)+".png";
  curl_easy_setopt(curl,CURLOPT_URL,url.c_str());curl_easy_setopt(curl,CURLOPT_USERAGENT,"KondorAdvancedDecoder/" DX_VERSION " (+https://github.com/ReLLL/kondor-weak-signal-modes-decoder)");
  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&bytes);curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT_MS,2500L);curl_easy_setopt(curl,CURLOPT_TIMEOUT_MS,5000L);curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
  std::string responseHeaders;curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,headers);curl_easy_setopt(curl,CURLOPT_HEADERDATA,&responseHeaders);
  Request request{this,key};curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&request);curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_HTTP_VERSION,CURL_HTTP_VERSION_2TLS);
  ++requests;auto code=curl_easy_perform(curl);long response=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response);
  if(request.cancelled){std::lock_guard<std::mutex> lock(mutex);auto found=entries.find(key);if(found!=entries.end())found->second.retry=Clock::now();return {};}
  if(code!=CURLE_OK||response!=200){std::lock_guard<std::mutex> lock(mutex);error="Map tiles unavailable; offline map remains available in Settings.";return stale;}
  int w=0,h=0,c=0;if(!stbi_info_from_memory(bytes.data(),int(bytes.size()),&w,&h,&c)||w!=256||h!=256)return {};
  std::transform(responseHeaders.begin(),responseHeaders.end(),responseHeaders.begin(),[](unsigned char c){return char(std::tolower(c));});int64_t maxAge=86400;auto age=responseHeaders.find("max-age=");if(age!=std::string::npos){auto parsed=std::from_chars(responseHeaders.data()+age+8,responseHeaders.data()+responseHeaders.size(),maxAge);if(parsed.ec!=std::errc{})maxAge=86400;maxAge=std::clamp<int64_t>(maxAge,0,31536000);}
  if(responseHeaders.find("no-store")==std::string::npos){std::lock_guard<std::mutex> diskLock(diskMutex);auto previous=std::filesystem::exists(path,ec)?std::filesystem::file_size(path,ec):0;std::filesystem::create_directories(path.parent_path(),ec);if(!ec){auto temporary=path;temporary+=".tmp";std::ofstream out(temporary,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));out.close();if(out){std::filesystem::rename(temporary,path,ec);if(!ec){std::ofstream expiry(path.string()+".expiry");expiry<<std::time(nullptr)+maxAge;diskBytes=uint64_t(std::max<int64_t>(0,int64_t(diskBytes.load())+int64_t(bytes.size())-int64_t(previous)));if(diskBytes>256*1024*1024)trimDisk();}}}}
  return bytes;
 }
 void trimDisk(){
  std::error_code ec;if(!std::filesystem::exists(cache,ec))return;
  std::vector<std::pair<std::filesystem::file_time_type,std::filesystem::path>> files;uintmax_t total=0;
  for(std::filesystem::recursive_directory_iterator it(cache,ec),end;it!=end&&!ec;it.increment(ec))if(it->is_regular_file(ec)&&(it->path().extension()==".png"||it->path().extension()==".jpg")){total+=it->file_size(ec);files.emplace_back(it->last_write_time(ec),it->path());}
  diskBytes=total;if(total<=256*1024*1024)return;std::sort(files.begin(),files.end());for(const auto& entry:files){if(total<=192*1024*1024)break;auto size=std::filesystem::file_size(entry.second,ec);if(std::filesystem::remove(entry.second,ec)){total-=size;std::filesystem::remove(entry.second.string()+".expiry",ec);}}diskBytes=total;
 }
 void run(){
  std::call_once(initializeDisk,[this]{std::lock_guard<std::mutex> diskLock(diskMutex);trimDisk();});auto* curl=curl_easy_init();if(!curl)return;while(!stopping){TileKey key{};{
   std::unique_lock<std::mutex> lock(mutex);auto find=[&](){for(auto k:desired){auto& e=entries[k];if(!e.texture&&e.pixels.empty()&&e.retry<=Clock::now()){key=k;return true;}}return false;};
   if(!find()){cv.wait_for(lock,std::chrono::seconds(1));continue;}entries[key].retry=Clock::now()+std::chrono::seconds(60);
  }
  auto bytes=fetch(key,curl);if(stopping)break;int w=0,h=0,c=0;unsigned char* pixels=bytes.empty()?nullptr:stbi_load_from_memory(bytes.data(),int(bytes.size()),&w,&h,&c,4);
  {std::lock_guard<std::mutex> lock(mutex);if(pixels&&w==256&&h==256&&std::find(desired.begin(),desired.end(),key)!=desired.end()){entries[key].pixels.assign(pixels,pixels+256*256*4);error.clear();bool complete=true;for(auto k:desired){auto found=entries.find(k);if(found==entries.end()||(!found->second.texture&&found->second.pixels.empty())){complete=false;break;}}if(complete)viewportDone=Clock::now();}}
  stbi_image_free(pixels);
  }curl_easy_cleanup(curl);
 }
};
Tiles::Tiles(std::filesystem::path cache):impl_(std::make_unique<Impl>(std::move(cache))){}
Tiles::~Tiles(){impl_->stopping=true;impl_->cv.notify_all();for(auto& worker:impl_->workers)worker.join();for(auto& [_,e]:impl_->entries)if(e.texture)glDeleteTextures(1,&e.texture);}
void Tiles::visible(const std::vector<TileKey>& keys){std::lock_guard<std::mutex> lock(impl_->mutex);if(impl_->desired==keys)return;impl_->desired=keys;auto now=Clock::now();for(auto key:keys)impl_->entries[key].used=now;
 while(impl_->entries.size()>256){auto oldest=impl_->entries.end();for(auto it=impl_->entries.begin();it!=impl_->entries.end();++it)if(it->second.used<now&&(oldest==impl_->entries.end()||it->second.used<oldest->second.used))oldest=it;if(oldest==impl_->entries.end())break;if(oldest->second.texture)glDeleteTextures(1,&oldest->second.texture);impl_->entries.erase(oldest);}
 impl_->viewportStart=now;impl_->viewportDone=now;impl_->cv.notify_all();
}
unsigned int Tiles::texture(TileKey key){std::lock_guard<std::mutex> lock(impl_->mutex);auto found=impl_->entries.find(key);if(found==impl_->entries.end())return 0;auto& e=found->second;if(!e.texture&&!e.pixels.empty()){
 GLint previous=0,rowLength=0,alignment=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glGenTextures(1,&e.texture);glBindTexture(GL_TEXTURE_2D,e.texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,e.pixels.data());glBindTexture(GL_TEXTURE_2D,GLuint(previous));glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);e.pixels.clear();e.pixels.shrink_to_fit();
 }return e.texture;
}
std::string Tiles::status(){std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->error;}
std::vector<TileKey> Tiles::ready(){std::lock_guard<std::mutex> lock(impl_->mutex);std::vector<TileKey> keys;for(const auto& [key,e]:impl_->entries)if(e.texture||!e.pixels.empty())keys.push_back(key);return keys;}
TileStats Tiles::stats(){std::lock_guard<std::mutex> lock(impl_->mutex);size_t ready=0;for(auto key:impl_->desired){auto found=impl_->entries.find(key);if(found!=impl_->entries.end()&&(found->second.texture||!found->second.pixels.empty()))++ready;}auto done=ready==impl_->desired.size()?impl_->viewportDone:Clock::now();return {impl_->requests.load(),impl_->diskHits.load(),impl_->diskBytes.load(),impl_->entries.size(),impl_->desired.size(),ready,std::chrono::duration<double,std::milli>(done-impl_->viewportStart).count()};}
}
