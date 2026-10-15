// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <thread>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){
 if(argc!=3)return 2;auto pattern=(std::filesystem::temp_directory_path()/"dx-mode-recovery-XXXXXX").string();if(!mkdtemp(pattern.data()))return 2;std::filesystem::path folder=pattern;TestFolderCleanup cleanup(folder);
 std::filesystem::create_symlink(std::filesystem::absolute(argv[1]),folder/"dx_mock_valid_small");std::filesystem::create_symlink(std::filesystem::absolute(argv[2]),folder/"dx_modes_decoder");auto start=dx::monotonicMs();
 {
  dx::Engine engine(folder/"history",folder/"dx_mock_valid_small",{39,33},[](const std::string&){return std::make_pair("Test Country","NA");});
  dx::Job slow;slow.mode="FST4";slow.utcMs=int64_t(time(nullptr))*1000;slow.periodMs=15000;slow.dial=474200;slow.pcm.resize(180000);slow.deadlineMs=dx::monotonicMs()+120000;engine.enqueue(slow);
  auto wait=[&](auto condition){auto begin=dx::monotonicMs();while(dx::monotonicMs()-begin<5000){if(condition(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(20));}return condition(*engine.snapshot());};
  CHECK(wait([](const dx::Snapshot& s){return s.modeDecoding;}));for(int i=0;i<40;i++)engine.enqueue(slow);
  dx::Job fast=slow;fast.mode="FT8";fast.dial=7074000;engine.enqueue(std::move(fast));CHECK(wait([](const dx::Snapshot& s){return s.stats.messages==3;}));CHECK(engine.snapshot()->modeQueued==dx::additionalQueueLimit&&engine.snapshot()->skipped>=40-dx::additionalQueueLimit);
  start=dx::monotonicMs();engine.invalidateAdditional();CHECK(wait([](const dx::Snapshot& s){return !s.modeDecoding&&s.modeQueued==0;}));CHECK(dx::monotonicMs()-start<2000);CHECK(engine.snapshot()->stats.messages==3);
  engine.enqueue(slow);CHECK(wait([](const dx::Snapshot& s){return s.modeDecoding;}));start=dx::monotonicMs();
 }
 CHECK(dx::monotonicMs()-start<2000);std::cout<<"Independent FT worker, bounded additional queue, retune cancellation and fast shutdown passed\n";return 0;
}
