// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <array>
#include <vector>
#include <stdexcept>
namespace dx {
namespace {
struct Handle {HANDLE h=nullptr;~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
}
std::string windowsWsprOutput(const std::filesystem::path& binary,const std::string& dial,const std::filesystem::path& input){
 SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};Handle read,write;
 if(!CreatePipe(&read.h,&write.h,&security,0)||!SetHandleInformation(read.h,HANDLE_FLAG_INHERIT,0))throw std::runtime_error("WSPR output unavailable");
 Handle null{CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr)};
 if(null.h==INVALID_HANDLE_VALUE)throw std::runtime_error("WSPR input unavailable");
 STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;startup.StartupInfo.hStdOutput=write.h;startup.StartupInfo.hStdInput=startup.StartupInfo.hStdError=null.h;
 SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&bytes);std::vector<unsigned char> storage(bytes);startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
 if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&bytes))throw std::runtime_error("WSPR process setup failed");
 struct Attributes {LPPROC_THREAD_ATTRIBUTE_LIST p;~Attributes(){DeleteProcThreadAttributeList(p);}} attributes{startup.lpAttributeList};
 std::array<HANDLE,2> inherited{write.h,null.h};if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited.data(),sizeof(inherited),nullptr,nullptr))throw std::runtime_error("Cannot restrict WSPR handles");
 auto command=L"\""+binary.wstring()+L"\" -a . -f "+std::wstring(dial.begin(),dial.end())+L" \""+input.wstring()+L"\"";PROCESS_INFORMATION child{};
 if(!CreateProcessW(binary.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT|CREATE_NO_WINDOW|BELOW_NORMAL_PRIORITY_CLASS,nullptr,nullptr,&startup.StartupInfo,&child))throw std::runtime_error("WSPR decoder could not start");
 Handle process{child.hProcess},thread{child.hThread};CloseHandle(write.h);write.h=nullptr;
 // The isolated parent decoder's job also owns this child. Its outer deadline
 // kills the entire tree if native decoding stops responding.
 std::string output;bool overflow=false;char block[4096];DWORD count=0;
 while(ReadFile(read.h,block,sizeof(block),&count,nullptr)&&count){if(output.size()+count<=1024*1024)output.append(block,count);else overflow=true;}
 if(WaitForSingleObject(process.h,5000)!=WAIT_OBJECT_0){TerminateProcess(process.h,1);throw std::runtime_error("WSPR decoder did not exit");}
 DWORD exit=1;GetExitCodeProcess(process.h,&exit);if(exit||overflow)throw std::runtime_error("WSPR decoder failed");return output;
}
}
