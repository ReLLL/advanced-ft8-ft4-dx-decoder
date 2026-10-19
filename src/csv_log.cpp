// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "csv_log.h"
#include "helper_client.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#endif
namespace dx {
namespace {
std::string field(const std::string& value){std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';}
std::string stamp(int64_t utc){time_t time=utc;tm date{};platform::utcTime(&time,&date);char value[32];strftime(value,sizeof(value),"%Y-%m-%dT%H:%M:%SZ",&date);return value;}
int openLog(const std::filesystem::path& path,int flags){
#ifdef _WIN32
 return _wopen(path.c_str(),flags|_O_BINARY,_S_IREAD|_S_IWRITE);
#else
 return open(path.c_str(),flags,0600);
#endif
}
int flushLog(int fd){
#ifdef _WIN32
 return _commit(fd);
#else
 return fsync(fd);
#endif
}
struct File {int fd=-1;explicit File(const std::filesystem::path& path,int flags):fd(openLog(path,flags)){if(fd<0)throw std::runtime_error(std::strerror(errno));}~File(){close();}void close(){if(fd>=0){::close(fd);fd=-1;}}void write(const std::string& value){size_t offset=0;while(offset<value.size()){auto count=::write(fd,value.data()+offset,value.size()-offset);if(count<0&&errno==EINTR)continue;if(count<=0)throw std::runtime_error("CSV write failed");offset+=size_t(count);}}void flush(){if(flushLog(fd)!=0)throw std::runtime_error("CSV flush failed");}};
// A CSV record ends only at a newline outside quoted fields. Preserve escaped
// quotes while copying unchanged records verbatim.
template<class Callback> uint64_t records(std::ifstream& input,uint64_t start,Callback callback){
 bool quoted=false;int column=0;std::array<std::string,28> fields;uint64_t offset=start,recordStart=start;char c;
 while(input.get(c)){++offset;auto append=[&](char value){if(column>=28)throw std::runtime_error("CSV record has too many fields");fields[size_t(column)]+=value;};
  if(c=='"'&&quoted&&input.peek()=='"'){input.get(c);++offset;append('"');}
  else if(c=='"')quoted=!quoted;
  else if(c==','&&!quoted)++column;
  else if(quoted||(c!='\r'&&c!='\n'))append(c);
  if(c=='\n'&&!quoted){const auto& id=fields[0];if(column!=27||id.empty()||id.size()>19||id.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("CSV record has an invalid format");callback(recordStart,offset,std::stoll(id),fields);recordStart=offset;column=0;fields={};}
  if(offset-recordStart>2*1024*1024)throw std::runtime_error("CSV record is too large");
 }
 if(!input.eof())throw std::runtime_error("CSV read failed");return recordStart;
}
}
std::string csvLogHeader(){return "ReceptionId,UTC,UnixUTC,SlotMs,Call,Called,Country,Continent,Grid,GridSource,GridUTC,Mode,Band,DialHz,AudioHz,SNRdB,DTSeconds,ApproxKm,BearingDegrees,MessageType,Message,Raw,Source,IdentitySource,FirstStation,FirstCountry,FirstGrid,DistanceRecord\r\n";}
std::string csvLogRow(int64_t id,const Decode& d){
 std::ostringstream line;line.imbue(std::locale::classic());line<<std::setprecision(12)<<id<<','<<field(stamp(d.utc))<<','<<d.utc<<','<<d.slotMs<<','<<field(d.identity.transmitter)<<','<<field(d.identity.called)<<','<<field(d.country)<<','<<field(d.continent)<<','<<field(d.identity.grid)<<','<<field(d.gridSource)<<','<<d.gridUtc<<','<<field(d.mode)<<','<<field(bandName(d.dial))<<','<<d.dial<<','<<d.audio<<','<<d.snr<<',';
 if(d.dt)line<<*d.dt;line<<',';if(d.km)line<<*d.km;line<<',';if(d.bearing)line<<*d.bearing;
 line<<','<<field(d.identity.type)<<','<<field(d.message)<<','<<field(d.raw)<<','<<field(d.source)<<','<<field(d.identitySource)<<','<<d.firstCall<<','<<d.firstCountry<<','<<d.firstGrid<<','<<d.distanceRecord<<"\r\n";return line.str();
}
void CsvLog::recover(){
 auto header=csvLogHeader();std::ifstream input(path_,std::ios::binary);std::string actual(header.size(),'\0');input.read(actual.data(),actual.size());if(actual!=header)throw std::runtime_error("Choose a new CSV file; this file is not a reception log");
 cursor_=0;auto end=records(input,header.size(),[&](uint64_t,uint64_t,int64_t id,const std::array<std::string,28>&){if(id<=cursor_)throw std::runtime_error("CSV reception IDs are out of order");cursor_=id;});input.close();
 if(end<std::filesystem::file_size(path_)){std::filesystem::resize_file(path_,end);File file(path_,O_WRONLY);file.flush();}bytes_=end;ready_=true;
}
void CsvLog::trim(uint64_t maximumBytes){
 if(bytes_<=maximumBytes)return;auto header=csvLogHeader();uint64_t target=maximumBytes*85/100,cut=0,last=header.size();std::ifstream input(path_,std::ios::binary);input.seekg(header.size());
 records(input,header.size(),[&](uint64_t start,uint64_t,int64_t,const std::array<std::string,28>&){last=start;if(!cut&&bytes_-start+header.size()<=target)cut=start;});if(!cut)cut=last;
 auto temporary=path_;temporary+=".trimming-"+std::to_string(getpid());std::error_code error;if(std::filesystem::exists(temporary))throw std::runtime_error("CSV temporary file already exists");
 try{File output(temporary,O_WRONLY|O_CREAT|O_EXCL);output.write(header);input.clear();input.seekg(cut);char buffer[65536];while(input.read(buffer,sizeof(buffer))||input.gcount()){output.write(std::string(buffer,size_t(input.gcount())));}if(!input.eof())throw std::runtime_error("CSV trim read failed");output.flush();output.close();input.close();std::filesystem::rename(temporary,path_);bytes_=header.size()+bytes_-cut;}catch(...){std::filesystem::remove(temporary,error);throw;}
}
int64_t CsvLog::purge(Store& store,const std::filesystem::path& path,const std::string& call,int64_t throughId){return purge(store,path,IgnoreTarget{upper(call),false},throughId);}
int64_t CsvLog::purge(Store& store,const std::filesystem::path& path,const IgnoreTarget& target,int64_t throughId){return purgeMatching(store,path,throughId,[&](const auto& fields){return upper(fields[target.message?20:4])==target.value;});}
int64_t CsvLog::purgeSaved(Store& store,const std::filesystem::path& path,const SavedCleanup& cleanup,int64_t throughId){
 if(!cleanup.valid())throw std::runtime_error("Invalid saved-message cleanup");
 return purgeMatching(store,path,throughId,[&](const auto& fields){Decode d;d.utc=std::stoll(fields[2]);d.country=countryName(fields[6]);d.identity.grid=fields[8];d.gridSource=fields[9];if(!fields[17].empty())d.km=std::stod(fields[17]);return cleanup.matches(d);});
}
int64_t CsvLog::purgeMatching(Store& store,const std::filesystem::path& path,int64_t throughId,const std::function<bool(const std::array<std::string,28>&)>& matches){
 auto resolved=std::filesystem::absolute(path).lexically_normal();if(!std::filesystem::exists(resolved))return 0;
 if(std::filesystem::is_symlink(resolved))throw std::runtime_error("CSV purge refuses a symbolic link");
 auto header=csvLogHeader();std::ifstream input(resolved,std::ios::binary),copy(resolved,std::ios::binary);std::string actual(header.size(),'\0');input.read(actual.data(),actual.size());if(actual!=header)throw std::runtime_error("CSV purge refuses an unrecognized log header");
 auto temporary=resolved;temporary+=".purging-"+std::to_string(getpid())+"-"+std::to_string(monotonicMs());int64_t removed=0,lastId=0;bool created=false;std::error_code error;
 try{File output(temporary,O_WRONLY|O_CREAT|O_EXCL);created=true;output.write(header);
  auto end=records(input,header.size(),[&](uint64_t start,uint64_t finish,int64_t id,const std::array<std::string,28>& fields){
   if(id<=lastId)throw std::runtime_error("CSV reception IDs are out of order");lastId=id;
   if(id<=throughId&&matches(fields)){++removed;return;}
   copy.clear();copy.seekg(start);std::string record(size_t(finish-start),'\0');copy.read(record.data(),record.size());if(size_t(copy.gcount())!=record.size())throw std::runtime_error("CSV purge read failed");output.write(record);
  });
  if(end!=std::filesystem::file_size(resolved))throw std::runtime_error("CSV purge found an incomplete record");
  output.flush();output.close();input.close();copy.close();
  if(!removed){std::filesystem::remove(temporary);return 0;}
  // Keep the original append watermark when deletion removes a trimmed log's tail.
  // Save it before rename so a crash cannot resurrect previously trimmed records.
  auto floors=store.getSetting("csv_floors",Json::object());floors[resolved.string()]=std::max(floors.value(resolved.string(),int64_t(0)),lastId);store.setSetting("csv_floors",floors);
  std::filesystem::rename(temporary,resolved);if(path_==resolved)ready_=false;return removed;
 }catch(...){if(created)std::filesystem::remove(temporary,error);throw;}
}
bool CsvLog::sync(Store& store,const std::filesystem::path& path,uint64_t maximumBytes){
 if(maximumBytes<1024*1024)throw std::runtime_error("CSV limit must be at least 1 MB");auto resolved=std::filesystem::absolute(path).lexically_normal();if(resolved.filename().empty())throw std::runtime_error("Choose a CSV filename");
 if(path_!=resolved){path_=resolved;ready_=false;cursor_=0;bytes_=0;}
 if(!std::filesystem::exists(path_)){std::filesystem::create_directories(path_.parent_path());File file(path_,O_WRONLY|O_CREAT|O_EXCL);file.write(csvLogHeader());file.flush();ready_=false;}
 if(!ready_||std::filesystem::file_size(path_)!=bytes_)recover();
 auto floors=store.getSetting("csv_floors",Json::object());cursor_=std::max(cursor_,floors.value(path_.string(),int64_t(0)));
 auto paths=store.getSetting("csv_paths",Json::array());if(std::find(paths.begin(),paths.end(),path_.string())==paths.end()){paths.push_back(path_.string());store.setSetting("csv_paths",paths);}
 auto page=store.logRows(cursor_,256);std::string buffer;int64_t next=cursor_;
 for(const auto& entry:page){auto row=csvLogRow(entry.first,entry.second);if(row.size()+csvLogHeader().size()>maximumBytes)throw std::runtime_error("Increase the CSV size limit to fit this reception");if(!buffer.empty()&&buffer.size()+row.size()>1024*1024)break;buffer+=row;next=entry.first;}
 if(!buffer.empty()){try{File file(path_,O_WRONLY|O_APPEND);file.write(buffer);file.flush();bytes_+=buffer.size();cursor_=next;}catch(...){ready_=false;throw;}}
 trim(maximumBytes);return !page.empty();
}
}
