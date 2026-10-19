// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "model.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <thread>
namespace {
bool transfer(int fd,void* data,size_t size,bool writing){auto* p=static_cast<char*>(data);while(size){int n=writing?_write(fd,p,unsigned(std::min<size_t>(size,65536))):_read(fd,p,unsigned(std::min<size_t>(size,65536)));if(n<=0)return false;size-=n;p+=n;}return true;}
}
int main(){
 _setmode(0,_O_BINARY);_setmode(1,_O_BINARY);wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);auto binary=std::filesystem::path(path);auto name=binary.filename().wstring();
 if(name.find(L"crash")!=std::wstring::npos)return 3;
 if(name.find(L"parent-hang")!=std::wstring::npos){
  auto child=binary.parent_path()/L"child-hang.exe";auto command=L"\""+child.wstring()+L"\"";STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION p{};
  if(!CreateProcessW(child.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&p))return 4;
  wchar_t record[32768];if(GetEnvironmentVariableW(L"DX_TEST_CHILD_PID_FILE",record,32768))std::ofstream(std::filesystem::path(record))<<p.dwProcessId;
  CloseHandle(p.hThread);CloseHandle(p.hProcess);
 }
 if(name.find(L"hang")!=std::wstring::npos){Sleep(60000);return 0;}
 for(;;){uint32_t bytes=0;if(!transfer(0,&bytes,4,false))return 0;if(bytes>4096)return 2;std::string header(bytes,'\0');if(!transfer(0,header.data(),bytes,false))return 2;
  auto request=dx::Json::parse(header);auto count=request.at("samples").get<size_t>();if(count>180000)return 2;std::vector<int16_t> pcm(count);if(!transfer(0,pcm.data(),count*2,false))return 2;
  auto id=request.at("id").get<int64_t>();if(name.find(L"stale")!=std::wstring::npos)++id;
  auto result=dx::Json{{"id",id},{"results",dx::Json::array({{{"message","CQ A1TEST FN42"},{"snr",-10},{"audio_hz",1500}}})}}.dump();bytes=uint32_t(result.size());if(!transfer(1,&bytes,4,true)||!transfer(1,result.data(),bytes,true))return 3;
 }
}
