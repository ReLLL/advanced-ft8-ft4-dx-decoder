// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "helper_client.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <sddl.h>
#include <objbase.h>
#include <array>
#include <stdexcept>
namespace dx {
namespace {
struct Handle {
 HANDLE value=nullptr;
 explicit Handle(HANDLE h=nullptr):value(h){}
 ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
 Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
 HANDLE release(){auto h=value;value=nullptr;return h;}
};
std::wstring unique(){GUID g{};if(FAILED(CoCreateGuid(&g)))throw std::runtime_error("Cannot create a private decoder pipe");wchar_t text[40];StringFromGUID2(g,text,40);return text;}
struct Pipe {HANDLE server=nullptr,client=nullptr;};
Pipe pipe(bool writing){
 Handle token;HANDLE raw=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&raw))throw std::runtime_error("Cannot identify decoder owner");token.value=raw;
 DWORD size=0;GetTokenInformation(token.value,TokenUser,nullptr,0,&size);std::vector<unsigned char> data(size);
 if(!GetTokenInformation(token.value,TokenUser,data.data(),size,&size))throw std::runtime_error("Cannot identify decoder owner");
 wchar_t* sid=nullptr;if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid,&sid))throw std::runtime_error("Cannot secure decoder pipe");
 std::wstring descriptor=L"D:P(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);PSECURITY_DESCRIPTOR security=nullptr;
 if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(),SDDL_REVISION_1,&security,nullptr))throw std::runtime_error("Cannot secure decoder pipe");
 SECURITY_ATTRIBUTES attributes{sizeof(attributes),security,FALSE};auto name=L"\\\\.\\pipe\\AdvancedDx-"+unique();
 Handle server(CreateNamedPipeW(name.c_str(),(writing?PIPE_ACCESS_OUTBOUND:PIPE_ACCESS_INBOUND)|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
  PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,65536,65536,0,&attributes));LocalFree(security);
 if(server.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Decoder pipe unavailable");
 SECURITY_ATTRIBUTES inherited{sizeof(inherited),nullptr,TRUE};Handle client(CreateFileW(name.c_str(),writing?GENERIC_READ:GENERIC_WRITE,0,&inherited,OPEN_EXISTING,0,nullptr));
 if(client.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Decoder pipe connection unavailable");
 Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.value)throw std::runtime_error("Decoder connection event unavailable");
 OVERLAPPED connected{};connected.hEvent=event.value;
 if(!ConnectNamedPipe(server.value,&connected)){
  auto error=GetLastError();if(error==ERROR_IO_PENDING){DWORD count=0;if(WaitForSingleObject(event.value,5000)!=WAIT_OBJECT_0){CancelIoEx(server.value,&connected);GetOverlappedResult(server.value,&connected,&count,TRUE);throw std::runtime_error("Decoder connection timed out");}if(!GetOverlappedResult(server.value,&connected,&count,FALSE))throw std::runtime_error("Decoder connection failed");}
  else if(error!=ERROR_PIPE_CONNECTED)throw std::runtime_error("Decoder connection failed");
 }
 return {server.release(),client.release()};
}
}
void Helper::stop(){
 if(job_){TerminateJobObject(job_,1);CloseHandle(job_);job_=nullptr;}
 if(process_){WaitForSingleObject(process_,5000);CloseHandle(process_);process_=nullptr;}
 for(auto h:{input_,output_})if(h!=-1)CloseHandle(reinterpret_cast<HANDLE>(h));input_=output_=-1;pid_=-1;
 if(!work_.empty()){std::error_code ec;std::filesystem::remove_all(work_,ec);work_.clear();}
}
void Helper::start(){
 try{
  if(isolated_){auto path=std::filesystem::temp_directory_path()/(L"AdvancedDx-worker-"+unique());if(!std::filesystem::create_directory(path))throw std::runtime_error("Decoder scratch unavailable");work_=path;}
  auto in=pipe(true);Handle input(in.server),childInput(in.client);auto out=pipe(false);Handle output(out.server),childOutput(out.client);
  SECURITY_ATTRIBUTES inherited{sizeof(inherited),nullptr,TRUE};Handle null(CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&inherited,OPEN_EXISTING,0,nullptr));
  if(null.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Decoder diagnostics unavailable");
  STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
  startup.StartupInfo.hStdInput=childInput.value;startup.StartupInfo.hStdOutput=childOutput.value;startup.StartupInfo.hStdError=null.value;
  SIZE_T size=0;InitializeProcThreadAttributeList(nullptr,1,0,&size);std::vector<unsigned char> storage(size);startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
  if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&size))throw std::runtime_error("Decoder process setup failed");
  struct Attributes{LPPROC_THREAD_ATTRIBUTE_LIST list;~Attributes(){DeleteProcThreadAttributeList(list);}} attributes{startup.lpAttributeList};
  std::array<HANDLE,3> handles{childInput.value,childOutput.value,null.value};
  if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles.data(),sizeof(handles),nullptr,nullptr))throw std::runtime_error("Cannot restrict decoder handles");
  Handle job(CreateJobObjectW(nullptr,nullptr));JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!job.value||!SetInformationJobObject(job.value,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Cannot isolate decoder process");
  PROCESS_INFORMATION process{};std::wstring command=L"\""+binary_.wstring()+L"\"";
  if(!CreateProcessW(binary_.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT|CREATE_SUSPENDED|CREATE_NO_WINDOW|BELOW_NORMAL_PRIORITY_CLASS,
    nullptr,work_.empty()?binary_.parent_path().c_str():work_.c_str(),&startup.StartupInfo,&process))throw std::runtime_error("Decoder helper could not start");
  Handle owned(process.hProcess),thread(process.hThread);
  if(!AssignProcessToJobObject(job.value,owned.value)){TerminateProcess(owned.value,1);throw std::runtime_error("Decoder isolation failed");}
  if(ResumeThread(thread.value)==DWORD(-1)){TerminateProcess(owned.value,1);throw std::runtime_error("Decoder helper could not resume");}
  input_=reinterpret_cast<intptr_t>(input.release());output_=reinterpret_cast<intptr_t>(output.release());process_=owned.release();job_=job.release();pid_=int(process.dwProcessId);
 }catch(...){stop();throw;}
}
void Helper::transfer(HelperDescriptor descriptor,void* data,size_t size,bool writing,int64_t deadline,const std::atomic<bool>& stopping){
 auto handle=reinterpret_cast<HANDLE>(descriptor);auto* cursor=static_cast<unsigned char*>(data);
 Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));if(!event.value)throw std::runtime_error("Decoder IO event unavailable");
 while(size){
  if(stopping||monotonicMs()>deadline)throw std::runtime_error(stopping?"Stopping":"Decoder timed out");
  OVERLAPPED pending{};pending.hEvent=event.value;ResetEvent(event.value);DWORD count=0;auto amount=DWORD(std::min<size_t>(size,65536));
  BOOL ok=writing?WriteFile(handle,cursor,amount,&count,&pending):ReadFile(handle,cursor,amount,&count,&pending);
  if(!ok&&GetLastError()!=ERROR_IO_PENDING)throw std::runtime_error("Decoder helper exited");
  if(!ok){
   for(;;){auto result=WaitForSingleObject(event.value,100);if(result==WAIT_OBJECT_0)break;
    if(result!=WAIT_TIMEOUT||stopping||monotonicMs()>deadline){CancelIoEx(handle,&pending);GetOverlappedResult(handle,&pending,&count,TRUE);throw std::runtime_error(stopping?"Stopping":"Decoder timed out");}
   }
   if(!GetOverlappedResult(handle,&pending,&count,FALSE))throw std::runtime_error("Decoder pipe failed");
  }
  if(!count)throw std::runtime_error("Decoder helper exited");cursor+=count;size-=count;
 }
}
}
