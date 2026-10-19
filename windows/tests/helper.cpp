// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "helper_client.h"
#include <windows.h>
#include <fstream>
#include <thread>
#include <iostream>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(0)
int main(int argc,char** argv){try{
 CHECK(argc==2);auto fixture=std::filesystem::u8path(argv[1]);auto root=std::filesystem::temp_directory_path()/(L"AdvancedDx-helper-test-"+std::to_wstring(GetCurrentProcessId()));CHECK(!std::filesystem::exists(root));std::filesystem::create_directory(root);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};
 for(const wchar_t* name:{L"normal.exe",L"parent-hang.exe",L"child-hang.exe",L"stale.exe",L"crash.exe"})std::filesystem::copy_file(fixture,root/name);
 dx::Job job;job.id=4;job.utcMs=1700000010000;job.mode="FT8";job.dial=14074000;job.periodMs=15000;job.pcm.resize(180000);for(size_t i=0;i<job.pcm.size();i++)job.pcm[i]=int16_t(i);std::atomic<bool> cancel{false};
 {
  dx::Helper helper(root/L"normal.exe",true,2000);auto result=helper.decode(job,cancel);CHECK(result.at("results").size()==1);auto pid=helper.pid();CHECK(pid>0);
  CHECK(helper.decode(job,cancel).at("id")==job.id&&helper.pid()==pid);
 }
 for(const wchar_t* name:{L"stale.exe",L"crash.exe"}){dx::Helper helper(root/name,true,1000);bool failed=false;try{helper.decode(job,cancel);}catch(const std::exception&){failed=true;}CHECK(failed&&helper.pid()==-1);}
 auto record=root/L"child.pid";_wputenv_s(L"DX_TEST_CHILD_PID_FILE",record.c_str());
 {
  dx::Helper helper(root/L"parent-hang.exe",true,500);auto before=dx::monotonicMs();bool failed=false;try{helper.decode(job,cancel);}catch(const std::exception&){failed=true;}CHECK(failed&&helper.pid()==-1&&dx::monotonicMs()-before<3000);
  DWORD child=0;std::ifstream(record)>>child;CHECK(child>0);auto process=OpenProcess(SYNCHRONIZE,FALSE,child);if(process){CHECK(WaitForSingleObject(process,1000)==WAIT_OBJECT_0);CloseHandle(process);}
 }
 _wputenv_s(L"DX_TEST_CHILD_PID_FILE",L"");
 {
  dx::Helper helper(root/L"child-hang.exe",true,5000);std::thread stop([&]{std::this_thread::sleep_for(std::chrono::milliseconds(100));cancel=true;});bool failed=false;auto before=dx::monotonicMs();try{helper.decode(job,cancel);}catch(const std::exception&){failed=true;}stop.join();CHECK(failed&&dx::monotonicMs()-before<3000);
 }
 std::cout<<"Binary IPC, helper reuse, stale/crash recovery, cancellation and descendant cleanup passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
