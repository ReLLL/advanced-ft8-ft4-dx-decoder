// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){
 CHECK(argc==2);
 auto path=(std::filesystem::temp_directory_path()/"dx-alert-distance-XXXXXX").string();CHECK(mkdtemp(path.data()));TestFolderCleanup cleanup(path);dx::Store store(std::filesystem::path(path)/"history.sqlite3");
 auto home=*dx::gridPoint("KM69");int64_t utc=1800000000;dx::Decode far;far.utc=utc;far.mode="FT8";far.dial=14074000;far.message="CQ EX7CQ MN72";far.source="live";CHECK(store.insert(far,home));
 auto repeat=far;repeat.utc+=15;repeat.message="DB6IF EX7CQ RR73";CHECK(store.insert(repeat,home));CHECK(repeat.gridSource=="cached");
 auto scope=store.sessionStats(utc+45,0,utc);CHECK(scope.farthestCall=="EX7CQ"&&scope.farthest>3500);
 dx::Query q;q.since=utc-3600;q.filter.band="20m";auto latest=store.rows(q);CHECK(latest.size()==1&&latest.front().decode.gridSource=="cached");
 dx::Alerts alerts;alerts.baseline(latest);alerts.distanceBaseline(scope.farthest);dx::AlertPolicy policy;policy.filter=q.filter;
 dx::Decode near;near.utc=utc+45;near.mode="FT8";near.dial=14074000;near.message="CQ RA5AD KO85";near.source="live";CHECK(store.insert(near,home));CHECK(*near.km<scope.farthest);
 auto result=alerts.evaluate({near},policy,near.utc);if(result)std::cerr<<result->text<<" although EX7CQ is "<<scope.farthest<<" km\n";
 CHECK(!result);
 // A wider statistics scope still protects the record outside alert filters.
 alerts.baseline({});alerts.distanceBaseline(scope.farthest);near.utc+=60;
 CHECK(!alerts.evaluate({near},policy,near.utc));
 auto farther=far;farther.utc=near.utc+60;farther.message="CQ JA1ABC PM95";CHECK(store.insert(farther,home));CHECK(*farther.km>scope.farthest);
 alerts.distanceBaseline(scope.farthest);result=alerts.evaluate({farther},policy,farther.utc);CHECK(result&&result->text.find("New distance record: JA1ABC")==0);
 // Clearing the statistics scope permits a new first record.
 alerts.distanceBaseline(0);near.utc=farther.utc+60;CHECK(alerts.evaluate({near},policy,near.utc));
 // Exercise the actual Engine call site, not just a seeded Alerts object.
 auto root=std::filesystem::path(path)/"engine";std::filesystem::create_directory(root);auto mock=root/"dx_mock_valid_small";std::filesystem::create_symlink(std::filesystem::absolute(argv[1]),mock);const auto current=int64_t(time(nullptr));
 {dx::Store saved(root/"history.sqlite3");auto d=far;d.utc=current-60;d.message="CQ VK2ABC QF56";CHECK(saved.insert(d,home));d.utc=current-45;d.message="JA1ABC VK2ABC RR73";CHECK(saved.insert(d,home)&&d.gridSource=="cached");saved.setSetting("ui",{{"history_all",true}});}
 {dx::Engine engine(root,mock,home,[](const auto&){return std::make_pair(std::string{},std::string{});});auto wait=[&](auto predicate){auto until=dx::monotonicMs()+5000;while(dx::monotonicMs()<until){if(predicate(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return predicate(*engine.snapshot());};CHECK(wait([](const auto& s){return s.generation>0&&s.historyAll;}));engine.active(true);engine.alertContext("20m",true,false);dx::Job job;job.mode="FT8";job.dial=14074000;job.utcMs=current*1000;job.deadlineMs=dx::monotonicMs()+30000;job.pcm.resize(180000);engine.enqueue(job);CHECK(wait([](const auto& s){return s.decodedJobs==1&&s.savedMessages==5;}));CHECK(engine.snapshot()->alert.empty()&&engine.snapshot()->stats.farthestCall=="VK2ABC");}
 return 0;
}
