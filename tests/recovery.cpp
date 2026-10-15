// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "helper_client.h"
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){if(argc!=2)return 2;auto mock=std::filesystem::absolute(argv[1]);auto pattern=(std::filesystem::temp_directory_path()/"dx-recovery-XXXXXX").string();if(!mkdtemp(pattern.data()))return 2;std::filesystem::path folder=pattern;TestFolderCleanup cleanup(folder);for(auto suffix:{"crash","hang","stale","valid","valid_small"})std::filesystem::create_symlink(mock,folder/(std::string("dx_mock_")+suffix));
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
  CHECK(waitFor([](const dx::Snapshot& s){return s.generation>0;}));auto blank=engine.snapshot();CHECK(blank->table.empty()&&blank->map.empty()&&blank->paths.empty()&&blank->stats.messages==0&&blank->stats.countries==0&&blank->stats.stations==0&&blank->stats.farthest==0&&blank->stats.activeSeconds==0&&blank->stats.latest.empty()&&blank->cornerReceptions.empty());
  engine.active(true);CHECK(waitFor([](const dx::Snapshot& s){return s.stats.activeSeconds>.1;}));
  job.utcMs+=15000;job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);CHECK(waitFor([](const dx::Snapshot& s){return s.stats.messages==3&&s.pendingWrites==0;}));CHECK(engine.snapshot()->stats.countries==1&&engine.snapshot()->stats.stations==3&&engine.snapshot()->stats.newestCountries.size()==1);for(const auto& row:engine.snapshot()->table)CHECK(row.firstHeardUtc==job.utcMs/1000&&row.stationFirstHeardUtc==job.utcMs/1000);
  CHECK(!engine.snapshot()->statsHistory&&engine.snapshot()->cornerReceptions.size()==3);
  auto tableSize=engine.snapshot()->table.size();auto start=dx::monotonicMs();engine.settings({{"stats_history",true}});CHECK(dx::monotonicMs()-start<100);
  CHECK(waitFor([](const dx::Snapshot& s){return s.statsHistory&&s.stats.messages==1203;}));CHECK(engine.snapshot()->table.size()==tableSize&&engine.snapshot()->stats.activeSeconds>0);
  engine.settings({{"stats_history",false}});CHECK(waitFor([](const dx::Snapshot& s){return !s.statsHistory&&s.stats.messages==3;}));CHECK(engine.snapshot()->table.size()==tableSize);
  start=dx::monotonicMs();engine.settings({{"history_all",true}});CHECK(dx::monotonicMs()-start<100);
  CHECK(waitFor([](const dx::Snapshot& s){return s.historyAll&&s.statsHistory&&s.stats.messages==1203&&s.tableCount==1200&&s.mapCount==1200;}));CHECK(engine.snapshot()->cornerReceptions.size()==3);
  auto restored=engine.snapshot();CHECK(restored->table.size()==500&&restored->map.size()==1200&&restored->stats.activeSeconds>0);for(const auto& row:restored->table)CHECK(row.stationFirstHeardUtc==job.utcMs/1000-15&&row.firstHeardUtc==job.utcMs/1000-15);
  auto since=job.utcMs/1000+1;engine.settings({{"history_table_cleared",since},{"history_map_cleared",since}});CHECK(waitFor([](const dx::Snapshot& s){return s.historyAll&&s.tableCount==0&&s.mapCount==0&&s.stats.messages==1203;}));
  engine.settings({{"history_table_cleared",0},{"history_map_cleared",0}});CHECK(waitFor([](const dx::Snapshot& s){return s.historyAll&&s.tableCount==1200&&s.mapCount==1200;}));
  dx::Query filtered;filtered.filter.call="K1ABC";engine.query(filtered,filtered,2);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==2&&s.historyAll&&s.tableCount==1&&s.mapCount==1&&s.stats.messages==1203;}));
  dx::Query sessionView;sessionView.since=job.utcMs/1000;engine.query(sessionView,sessionView,3);engine.settings({{"history_all",false}});CHECK(waitFor([](const dx::Snapshot& s){return s.generation==3&&!s.historyAll&&!s.statsHistory&&s.stats.messages==3&&s.tableCount==3&&s.mapCount==3;}));
  engine.active(false);engine.resetStats();CHECK(waitFor([](const dx::Snapshot& s){return s.stats.messages==0&&s.stats.activeSeconds==0;}));CHECK(engine.snapshot()->newestReceptionUtc==0&&engine.snapshot()->stats.newestCountries.empty()&&engine.snapshot()->cornerReceptions.empty());
  engine.query(dx::Query{},dx::Query{},5);engine.settings({{"history_all",true}});CHECK(waitFor([](const dx::Snapshot& s){return s.historyAll&&s.mapCount==1200&&s.stats.messages==1203;}));
 }
 {dx::Engine engine(folder/"drain",folder/"dx_mock_valid_small",{39,33},[](const std::string&){return std::make_pair("Test Country","NA");});auto start=dx::monotonicMs();while(engine.snapshot()->generation==0&&dx::monotonicMs()-start<9000)std::this_thread::sleep_for(std::chrono::milliseconds(10));auto loaded=engine.snapshot();CHECK(loaded->historyAll&&loaded->statsHistory&&loaded->mapCount==1200&&loaded->tableCount==1200&&loaded->stats.messages==1203&&loaded->stats.activeSeconds==0);
  engine.settings({{"history_all",false}});start=dx::monotonicMs();while(engine.snapshot()->historyAll&&dx::monotonicMs()-start<9000)std::this_thread::sleep_for(std::chrono::milliseconds(10));auto fresh=engine.snapshot();CHECK(!fresh->historyAll&&!fresh->statsHistory&&fresh->table.empty()&&fresh->map.empty()&&fresh->stats.messages==0);
 }
 {dx::Store retained(folder/"drain/history.sqlite3");CHECK(retained.stats(0).messages==1203);dx::CsvLog log;CHECK(!log.sync(retained,folder/"drain/receptions.csv",100*1024*1024));CHECK(log.cursor()==1203);}
 {dx::Engine engine(folder/"tabs",folder/"dx_mock_valid_small",{39,33},[](const std::string&){return std::make_pair("Test Country","NA");});auto waitFor=[&](auto condition){auto start=dx::monotonicMs();while(dx::monotonicMs()-start<9000){if(condition(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return condition(*engine.snapshot());};CHECK(waitFor([](const dx::Snapshot& s){return s.generation>0;}));job.mode="FT8";job.utcMs=int64_t(time(nullptr))*1000;for(int i=0;i<3;i++){job.utcMs+=15000;job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);}CHECK(waitFor([](const dx::Snapshot& s){return s.stats.messages==9&&s.tableCount==3;}));
  dx::Query all;all.messages=true;all.sort=18;engine.query(all,dx::Query{},50);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==50&&s.tableMessages&&s.tableCount==9;}));CHECK(engine.snapshot()->table.size()==9&&engine.snapshot()->cornerReceptions.size()==9);for(const auto& row:engine.snapshot()->table)CHECK(row.count==1);
  dx::Query countries;countries.countries=true;countries.sort=5;engine.query(countries,dx::Query{},51);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==51&&s.tableCountries&&s.tableCount==1;}));CHECK(engine.snapshot()->countryTable.front().stations==3&&engine.snapshot()->countryTable.front().latestSenders.size()==3);
  engine.selectCountry("Test Country");CHECK(waitFor([](const dx::Snapshot& s){return s.detailsCountry=="Test Country"&&s.details.size()==9;}));CHECK(engine.snapshot()->details.front().utc==job.utcMs/1000);for(const auto& reception:engine.snapshot()->details)CHECK(reception.country=="Test Country");
  countries.filter.call="K1ABC";engine.query(countries,dx::Query{},53);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==53&&s.details.size()==3&&s.countryTable.front().stations==1;}));for(const auto& reception:engine.snapshot()->details)CHECK(reception.identity.transmitter=="K1ABC");
  countries.filter.country="Other Country";engine.query(countries,dx::Query{},54);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==54&&s.detailsCountry=="Test Country"&&s.details.empty();}));
  engine.selectCountry("Test Country",3);engine.query(dx::Query{},dx::Query{},55);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==55&&s.details.size()==6;}));
  engine.selectCountry("");CHECK(waitFor([](const dx::Snapshot& s){return s.detailsCountry.empty()&&s.details.empty();}));
  engine.query(dx::Query{},dx::Query{},52);CHECK(waitFor([](const dx::Snapshot& s){return s.generation==52&&!s.tableMessages&&!s.tableCountries&&s.tableCount==3&&s.stats.messages==9;}));
  engine.settings({{"corner_seconds",2}});CHECK(waitFor([](const dx::Snapshot& s){return s.cornerReceptions.empty();}));CHECK(engine.snapshot()->stats.messages==9&&engine.snapshot()->tableCount==3);
 }
 std::cout<<"Crash, 20-second hang, stale result, cancellation, queue bounds, failure circuit checks passed\n";return 0;
}
