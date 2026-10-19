// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <set>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto pattern=(std::filesystem::temp_directory_path()/"dx-activity-filters-XXXXXX").string();CHECK(mkdtemp(pattern.data()));TestFolderCleanup cleanup(pattern);
 const auto home=*dx::gridPoint("KM69");const int64_t now=time(nullptr);
 auto country=[](const std::string& call){return std::make_pair(call.rfind("JH",0)==0?"Japan":call.rfind("DL",0)==0?"Germany":"Other","AS");};
 {
  dx::Store store(std::filesystem::path(pattern)/"history.sqlite3");
  auto insert=[&](int age,int64_t frequency,const char* mode,int snr,const char* message){dx::Decode d;d.utc=now-age;d.slotMs=d.utc*1000;d.dial=frequency;d.mode=mode;d.snr=snr;d.message=message;return store.insert(d,home,country);};
  CHECK(insert(120,7074000,"FT8",-20,"CQ K1AAA FN42"));
  CHECK(insert(60,14074000,"FT8",-20,"CQ JH1AAA PM95"));
  CHECK(insert(30,7047500,"FT4",-5,"CQ DL1AAA JO33"));
  CHECK(insert(700,7074000,"FT8",-20,"CQ VK1AAA QF44"));
  CHECK(insert(1200,7074000,"FT8",-20,"CQ ZL1AAA RE78"));
  CHECK(insert(20,7074000,"FT8",-20,"CQ K2AAA")); // Country estimate is not location evidence.
  store.setSetting("ui",{{"history_all",true}});
 }
 dx::Engine engine(pattern,std::filesystem::path(pattern)/"unused-helper",home,country);
 // Capture/alert context must not override the shared reception filters.
 engine.alertContext("20m",true,false,"FT8");uint64_t generation=10;
 auto check=[&](dx::Query query,const std::set<std::string>& expected){
  const auto requested=++generation;engine.query(query,query,requested);
  auto until=dx::monotonicMs()+5000;
  while(dx::monotonicMs()<until){auto s=engine.snapshot();if(s->generation==requested&&s->historyAll){
   std::set<std::string> actual;for(const auto& d:s->directionStations)actual.insert(d.identity.transmitter);
   if(actual!=expected){std::cerr<<"Unexpected sample:";for(const auto& call:actual)std::cerr<<' '<<call;std::cerr<<'\n';return false;}
   auto bearing=dx::activityBearing(s->directionStations);return s->direction.stations==int(expected.size())&&s->direction.degrees==bearing.degrees;
  }std::this_thread::sleep_for(std::chrono::milliseconds(10));}return false;
 };
 dx::Query q;q.since=now-900;CHECK(check(q,{"K1AAA","JH1AAA","DL1AAA","VK1AAA"}));
 q.filter.band="40m";CHECK(check(q,{"K1AAA","DL1AAA","VK1AAA"}));
 q.since=now-300;CHECK(check(q,{"K1AAA","DL1AAA"}));
 q.filter.mode="FT8";CHECK(check(q,{"K1AAA"}));
 q.filter.mode.clear();q.until=now-90;CHECK(check(q,{"K1AAA"}));
 q.until=INT64_MAX;q.filter.country="Japan";CHECK(check(q,{}));
 q.filter.country.clear();q.filter.search="DL1AAA";CHECK(check(q,{"DL1AAA"}));
 q.filter.search.clear();q.filter.minSnr=-10;CHECK(check(q,{"DL1AAA"}));
 q.filter={};q.since=now-3600;CHECK(check(q,{"K1AAA","JH1AAA","DL1AAA","VK1AAA","ZL1AAA"}));
 q.until=now-1800;CHECK(check(q,{}));
 q={};engine.settings({{"history_map_cleared",now-45},{"history_table_cleared",now-45}});CHECK(check(q,{"DL1AAA"}));
 std::cout<<"Bearing/compass samples follow shared time, band, mode, search, country, SNR and history filters without an extra age limit\n";
}
