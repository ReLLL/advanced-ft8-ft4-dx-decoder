// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "helper_client.h"
#include "engine.h"
#include <iostream>
#include <unistd.h>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){if(argc!=2)return 2;auto mock=std::filesystem::absolute(argv[1]);auto folder=std::filesystem::temp_directory_path()/("dx-recovery-"+std::to_string(getpid()));std::filesystem::create_directory(folder);for(auto suffix:{"crash","hang","stale","valid","valid_small"})std::filesystem::create_symlink(mock,folder/(std::string("dx_mock_")+suffix));
 dx::Job job;job.id=1;job.mode="FT8";job.utcMs=1800000000000;job.pcm.resize(180000);std::atomic<bool> stopping{false};
 for(auto suffix:{"crash","stale","hang"}){dx::Helper helper(folder/(std::string("dx_mock_")+suffix));auto start=dx::monotonicMs();bool failed=false;try{helper.decode(job,stopping);}catch(const std::exception&){failed=true;}CHECK(failed&&helper.pid()==-1);if(std::string(suffix)=="hang")CHECK(dx::monotonicMs()-start>=20000&&dx::monotonicMs()-start<21500);}
 {dx::Helper helper(folder/"dx_mock_hang");auto start=dx::monotonicMs();std::thread cancel([&]{std::this_thread::sleep_for(std::chrono::milliseconds(250));stopping=true;});bool failed=false;try{helper.decode(job,stopping);}catch(const std::exception&){failed=true;}cancel.join();CHECK(failed&&dx::monotonicMs()-start<1000);}
 stopping=false;{
  dx::Engine engine(folder,folder/"dx_mock_crash",{39,33},[](const std::string&){return std::make_pair(std::string{},std::string{});});for(int i=0;i<40;i++){job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);}auto start=dx::monotonicMs();while(dx::monotonicMs()-start<9000){auto s=engine.snapshot();if(s->failures>=3){CHECK(s->skipped>=8&&!s->error.empty());break;}std::this_thread::sleep_for(std::chrono::milliseconds(100));}CHECK(engine.snapshot()->failures>=3);
 }
 {dx::Engine engine(folder/"drain",folder/"dx_mock_valid",{39,33},[](const std::string&){return std::make_pair("Test Country","NA");});job.utcMs=int64_t(time(nullptr))*1000;job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);auto start=dx::monotonicMs();while(engine.snapshot()->decodedJobs==0&&dx::monotonicMs()-start<6000)std::this_thread::sleep_for(std::chrono::milliseconds(1));CHECK(engine.snapshot()->decodedJobs==1);CHECK(engine.snapshot()->pendingWrites>0);}
 {dx::Store drained(folder/"drain/history.sqlite3");CHECK(drained.stats(0).messages==1200);dx::CsvLog log;CHECK(!log.sync(drained,folder/"drain/receptions.csv",100*1024*1024));CHECK(log.cursor()==1200);}
 {dx::Engine engine(folder/"drain",folder/"dx_mock_valid_small",{39,33},[](const std::string&){return std::make_pair("Test Country","NA");});
  auto waitFor=[&](auto condition){auto start=dx::monotonicMs();while(!condition(*engine.snapshot())&&dx::monotonicMs()-start<9000)std::this_thread::sleep_for(std::chrono::milliseconds(10));return condition(*engine.snapshot());};
  CHECK(waitFor([](const dx::Snapshot& s){return s.generation>0;}));auto blank=engine.snapshot();CHECK(blank->table.empty()&&blank->map.empty()&&blank->paths.empty()&&blank->stats.messages==0&&blank->stats.countries==0&&blank->stats.stations==0&&blank->stats.farthest==0&&blank->stats.activeSeconds==0&&blank->stats.latest.empty());
  engine.active(true);CHECK(waitFor([](const dx::Snapshot& s){return s.stats.activeSeconds>.1;}));
  job.utcMs+=15000;job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);CHECK(waitFor([](const dx::Snapshot& s){return s.stats.messages==3&&s.pendingWrites==0;}));CHECK(engine.snapshot()->stats.countries==1&&engine.snapshot()->stats.stations==3&&engine.snapshot()->stats.firstCountryKeys.size()==1);for(const auto& row:engine.snapshot()->table)CHECK(row.firstHeardUtc==job.utcMs/1000&&row.stationFirstHeardUtc==job.utcMs/1000);
  CHECK(!engine.snapshot()->statsHistory);
  auto tableSize=engine.snapshot()->table.size();auto start=dx::monotonicMs();engine.settings({{"stats_history",true}});CHECK(dx::monotonicMs()-start<100);
  CHECK(waitFor([](const dx::Snapshot& s){return s.statsHistory&&s.stats.messages==1203;}));CHECK(engine.snapshot()->table.size()==tableSize&&engine.snapshot()->stats.activeSeconds>0);
  engine.settings({{"stats_history",false}});CHECK(waitFor([](const dx::Snapshot& s){return !s.statsHistory&&s.stats.messages==3;}));CHECK(engine.snapshot()->table.size()==tableSize);
  engine.active(false);engine.resetStats();CHECK(waitFor([](const dx::Snapshot& s){return s.stats.messages==0&&s.stats.activeSeconds==0;}));CHECK(engine.snapshot()->newestReceptionUtc==0&&engine.snapshot()->stats.firstCountryKeys.empty());
 }
 {dx::Store retained(folder/"drain/history.sqlite3");CHECK(retained.stats(0).messages==1203);dx::CsvLog log;CHECK(!log.sync(retained,folder/"drain/receptions.csv",100*1024*1024));CHECK(log.cursor()==1203);}
 std::cout<<"Crash, 20-second hang, stale result, cancellation, queue bounds, failure circuit checks passed\n";return 0;
}
