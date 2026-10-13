// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "csv_log.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
namespace dx {
namespace {
std::string field(const std::string& value){std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';}
std::string stamp(int64_t utc){time_t time=utc;tm date{};gmtime_r(&time,&date);char value[32];strftime(value,sizeof(value),"%Y-%m-%dT%H:%M:%SZ",&date);return value;}
struct File {int fd=-1;explicit File(const std::filesystem::path& path,int flags):fd(open(path.c_str(),flags,0600)){if(fd<0)throw std::runtime_error(std::strerror(errno));}~File(){if(fd>=0)close(fd);}void write(const std::string& value){size_t offset=0;while(offset<value.size()){auto count=::write(fd,value.data()+offset,value.size()-offset);if(count<0&&errno==EINTR)continue;if(count<=0)throw std::runtime_error("CSV write failed");offset+=size_t(count);}}void flush(){if(fsync(fd)!=0)throw std::runtime_error("CSV flush failed");}};
// A CSV record ends only at a newline outside quoted fields. Doubled quotes
// toggle twice and therefore never expose embedded commas or newlines.
template<class Callback> uint64_t records(std::ifstream& input,uint64_t start,Callback callback){
 bool quoted=false;int columns=1;std::string id;uint64_t offset=start,recordStart=start;char c;
 while(input.get(c)){++offset;if(c=='\"')quoted=!quoted;else if(c==','&&!quoted)++columns;
  if(columns==1&&c!=','&&c!='\r'&&c!='\n'){if(c<'0'||c>'9'||id.size()>18)throw std::runtime_error("CSV reception ID is invalid");id+=c;}
  if(c=='\n'&&!quoted){if(columns!=28||id.empty())throw std::runtime_error("CSV record has an invalid format");callback(recordStart,offset,std::stoll(id));recordStart=offset;columns=1;id.clear();}
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
 cursor_=0;auto end=records(input,header.size(),[&](uint64_t,uint64_t,int64_t id){if(id<=cursor_)throw std::runtime_error("CSV reception IDs are out of order");cursor_=id;});input.close();
 if(end<std::filesystem::file_size(path_)){std::filesystem::resize_file(path_,end);File file(path_,O_WRONLY);file.flush();}bytes_=end;ready_=true;
}
void CsvLog::trim(uint64_t maximumBytes){
 if(bytes_<=maximumBytes)return;auto header=csvLogHeader();uint64_t target=maximumBytes*85/100,cut=0,last=header.size();std::ifstream input(path_,std::ios::binary);input.seekg(header.size());
 records(input,header.size(),[&](uint64_t start,uint64_t,int64_t){last=start;if(!cut&&bytes_-start+header.size()<=target)cut=start;});if(!cut)cut=last;
 auto temporary=path_;temporary+=".trimming-"+std::to_string(getpid());std::error_code error;if(std::filesystem::exists(temporary))throw std::runtime_error("CSV temporary file already exists");
 try{File output(temporary,O_WRONLY|O_CREAT|O_EXCL);output.write(header);input.clear();input.seekg(cut);char buffer[65536];while(input.read(buffer,sizeof(buffer))||input.gcount()){output.write(std::string(buffer,size_t(input.gcount())));}if(!input.eof())throw std::runtime_error("CSV trim read failed");output.flush();input.close();std::filesystem::rename(temporary,path_);bytes_=header.size()+bytes_-cut;}catch(...){std::filesystem::remove(temporary,error);throw;}
}
bool CsvLog::sync(Store& store,const std::filesystem::path& path,uint64_t maximumBytes){
 if(maximumBytes<1024*1024)throw std::runtime_error("CSV limit must be at least 1 MB");auto resolved=std::filesystem::absolute(path).lexically_normal();if(resolved.filename().empty())throw std::runtime_error("Choose a CSV filename");
 if(path_!=resolved){path_=resolved;ready_=false;cursor_=0;bytes_=0;}
 if(!std::filesystem::exists(path_)){std::filesystem::create_directories(path_.parent_path());File file(path_,O_WRONLY|O_CREAT|O_EXCL);file.write(csvLogHeader());file.flush();ready_=false;}
 if(!ready_||std::filesystem::file_size(path_)!=bytes_)recover();
 auto page=store.logRows(cursor_,256);std::string buffer;int64_t next=cursor_;
 for(const auto& entry:page){auto row=csvLogRow(entry.first,entry.second);if(row.size()+csvLogHeader().size()>maximumBytes)throw std::runtime_error("Increase the CSV size limit to fit this reception");if(!buffer.empty()&&buffer.size()+row.size()>1024*1024)break;buffer+=row;next=entry.first;}
 if(!buffer.empty()){try{File file(path_,O_WRONLY|O_APPEND);file.write(buffer);file.flush();bytes_+=buffer.size();cursor_=next;}catch(...){ready_=false;throw;}}
 trim(maximumBytes);return !page.empty();
}
}
