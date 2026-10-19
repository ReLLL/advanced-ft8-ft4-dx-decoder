// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "sharp_host.h"
#include <io.h>
#include <fcntl.h>
#include <cmath>
#include <windows.h>
namespace dx {
namespace {
bool transfer(int fd,void* data,size_t bytes,bool writing){auto* cursor=static_cast<char*>(data);while(bytes){int count=writing?_write(fd,cursor,unsigned(std::min<size_t>(bytes,65536))):_read(fd,cursor,unsigned(std::min<size_t>(bytes,65536)));if(count<=0)return false;bytes-=size_t(count);cursor+=count;}return true;}
}
SharpHost& sharpHost(){static SharpHost host;return host;}
void SharpHost::start(){_setmode(0,_O_BINARY);_setmode(1,_O_BINARY);reader_=std::thread(&SharpHost::read,this);writer_=std::thread(&SharpHost::write,this);}
void SharpHost::finish(){
 alive=false;cv_.notify_all();
 // Anonymous standard pipes are synchronous. Cancel blocked reads/writes even
 // when SDR# exits without closing its pipe handles in the normal order.
 auto cancel=[](unsigned long id){if(!id)return;auto thread=OpenThread(THREAD_TERMINATE,FALSE,id);if(thread){CancelSynchronousIo(thread);CloseHandle(thread);}};
 while((reader_.joinable()&&!readerDone_)||(writer_.joinable()&&!writerDone_)){
  if(!readerDone_)cancel(readerId_);if(!writerDone_)cancel(writerId_);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 if(reader_.joinable())reader_.join();if(writer_.joinable())writer_.join();
}
void SharpHost::plan(Json value){std::lock_guard<std::mutex> lock(mutex_);if(plan_==value)return;plan_=std::move(value);dirty_=true;cv_.notify_all();}
void SharpHost::read(){readerId_=GetCurrentThreadId();try{while(alive){
 uint32_t count=0;if(!transfer(0,&count,4,false)||!count||count>65536)break;std::string body(count,'\0');if(!transfer(0,body.data(),count,false))break;auto header=Json::parse(body);auto kind=header.at("kind").get<std::string>();
 if(kind=="metadata"){
  double c=header.at("center"),f=header.at("frequency"),r=header.at("sample_rate"),w=header.at("spectrum_width");if(!std::isfinite(c)||!std::isfinite(f)||!std::isfinite(r)||!std::isfinite(w)||r<0||r>100e6||c<0||c>1e12)break;
  center=c;frequency=f;rate=r;width=w;playing=header.at("playing").get<bool>();replay=header.at("replay").get<bool>();
 }else if(kind=="show")show=true;
 else if(kind=="reset"){if(consume)consume({{{"kind","reset"}}, {}});}
 else if(kind=="iq"){
  auto samples=header.at("samples").get<int64_t>();if(samples<=0||samples>131072)break;SharpAudio block{std::move(header),std::vector<std::complex<float>>(size_t(samples))};static_assert(sizeof(std::complex<float>)==8);
  if(!transfer(0,block.iq.data(),size_t(samples)*8,false))break;
  if(consume)consume(std::move(block));
 }else break;
}}catch(const std::exception&){}alive=false;readerDone_=true;cv_.notify_all();}
void SharpHost::write(){writerId_=GetCurrentThreadId();while(alive){Json value;{std::unique_lock<std::mutex> lock(mutex_);cv_.wait_for(lock,std::chrono::milliseconds(200),[&]{return dirty_||!alive;});if(!alive)break;if(!dirty_)continue;value=plan_;dirty_=false;}
 auto body=value.dump();uint32_t size=uint32_t(body.size());if(size>1024*1024||!transfer(1,&size,4,true)||!transfer(1,body.data(),size,true)){alive=false;break;}
}writerDone_=true;}
}
