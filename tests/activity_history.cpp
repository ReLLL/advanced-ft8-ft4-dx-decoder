// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto pattern=(std::filesystem::temp_directory_path()/"dx-activity-history-XXXXXX").string();CHECK(mkdtemp(pattern.data()));TestFolderCleanup cleanup(pattern);
 const auto home=*dx::gridPoint("KM69");const int64_t now=time(nullptr);
 {
  dx::Store store(std::filesystem::path(pattern)/"history.sqlite3");
  auto insert=[&](int age,int64_t dial,const char* mode,const char* message){dx::Decode d;d.utc=now-age;d.slotMs=d.utc*1000;d.dial=dial;d.mode=mode;d.snr=-23;d.message=message;d.source="fixture";return store.insert(d,home);};
  CHECK(insert(86400,7038600,"WSPR","MI0KOA IO74 40"));
  CHECK(insert(60,7038600,"WSPR","<TA4/G8SCU> KM56VO 37"));
  CHECK(insert(86400,14095600,"WSPR","VK7AAA QE38 37"));
  CHECK(insert(60,7074000,"FT8","CQ VK7BBB QE38"));
  store.setSetting("ui",{{"history_all",true},{"direction_stations",1}});
 }
 dx::Engine engine(pattern,std::filesystem::path(pattern)/"unused-helper",home,[](const auto&){return std::make_pair(std::string{},std::string{});});uint64_t generation=10;
 auto check=[&](dx::Query query,const std::string& expected){
  const auto requested=++generation;engine.query(query,query,requested);auto until=dx::monotonicMs()+5000;
  while(dx::monotonicMs()<until){auto s=engine.snapshot();if(s->generation==requested){if(expected.empty())return s->directionStations.empty()&&!s->direction.degrees;
   if(s->directionStations.size()!=1||s->directionStations.front().identity.transmitter!=expected){std::cerr<<"Expected "<<expected<<", got ";for(const auto& d:s->directionStations)std::cerr<<d.identity.transmitter<<' ';std::cerr<<'\n';return false;}
   auto station=s->directionStations.front();auto bearing=dx::distanceBearing(home,*station.position).second;return s->direction.stations==1&&s->direction.degrees&&std::abs(*s->direction.degrees-bearing)<1e-8;
  }std::this_thread::sleep_for(std::chrono::milliseconds(10));}return false;
 };
 dx::Query q;q.filter.mode="WSPR";q.filter.band="40m";CHECK(check(q,"MI0KOA"));
 CHECK(std::abs(*engine.snapshot()->direction.degrees-313.31829475931033)<1e-8);
 q.since=now-900;CHECK(check(q,"<TA4/G8SCU>"));
 q.since=now-2*86400;q.until=now-3600;CHECK(check(q,"MI0KOA"));
 q.filter.band="20m";CHECK(check(q,"VK7AAA"));
 q={};q.filter.mode="FT8";CHECK(check(q,"VK7BBB"));
 q.filter.mode="WSPR";q.filter.band="40m";q.filter.excludeCall="MI0KOA";CHECK(check(q,"<TA4/G8SCU>"));
 q.filter.excludeCall.clear();engine.settings({{"history_all",false}});CHECK(check(q,""));
 engine.settings({{"history_all",true}});CHECK(check(q,"MI0KOA"));
 std::cout<<"WSPR All/history selects MI0KOA at 3273 km and 313 degrees; shared windows, historical ranges, bands/modes and session scope apply\n";
}
