// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "model.h"
#include "engine.h"
#include "cartography.h"
#include <iostream>
#include <unistd.h>
#include <cmath>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 CHECK(dx::callsignLookupUrl("k1abc",0)=="https://www.google.com/search?q=K1ABC%20call%20sign");CHECK(dx::callsignLookupUrl("F/K1ABC/P",1)=="https://www.qrz.com/db/F%2FK1ABC%2FP");CHECK(dx::callsignLookupUrl("K1ABC",2)=="https://www.hamqth.com/K1ABC");CHECK(dx::callsignLookupUrl("<1:123>",0).empty()&&dx::callsignLookupUrl("K1ABC&test",0).empty()&&dx::callsignLookupUrl("K1ABC",3).empty());
 dx::Decode nearHome,farHome;nearHome.identity.transmitter="A1TEST";nearHome.position=dx::Point{0,10};nearHome.km=99999;nearHome.bearing=270;farHome.identity.transmitter="B1TEST";farHome.position=dx::Point{0,80};farHome.km=1;farHome.bearing=270;
 auto fromWest=dx::farthestActivity({0,0},{{nearHome,1},{farHome,1}},1);CHECK(fromWest.size()==1&&fromWest[0].identity.transmitter=="B1TEST"&&std::abs(*fromWest[0].bearing-90)<1e-6);
 auto fromEast=dx::farthestActivity({0,90},{{nearHome,1},{farHome,1}},1);CHECK(fromEast.size()==1&&fromEast[0].identity.transmitter=="A1TEST"&&std::abs(*fromEast[0].bearing-270)<1e-6);
 for(const auto& kind:std::vector<std::string>{"distance","station","country","grid","watch"}){dx::Decode heard;heard.utc=1800000000;heard.source="live";heard.identity.transmitter="K1ABC";heard.identity.grid="FN42";heard.country="United States";heard.km=8000;heard.gridSource="received";heard.firstCall=true;
  dx::AlertPolicy p;p.distance=false;p.station=kind=="station";p.country=kind=="country";p.grid=kind=="grid";p.watch=kind=="watch";p.watchCalls="K1*";p.distance=kind=="distance";p.sound=true;p.sounds={{kind,"Ping"}};dx::Alerts a;auto event=a.evaluate({heard},p,heard.utc);CHECK(event&&event->sound&&event->tone=="Ping"&&event->text.find('~')==std::string::npos);
  p.sounds[kind]="None";dx::Alerts silent;event=silent.evaluate({heard},p,heard.utc);CHECK(event&&!event->sound);p.sound=false;p.sounds[kind]="Glass";dx::Alerts muted;event=muted.evaluate({heard},p,heard.utc);CHECK(event&&!event->sound&&event->tone=="Glass");
 }
 CHECK(dx::alertTone({{"distance","bad"}},"distance")=="Hero");CHECK(dx::alertTone({{"distance",12}},"distance")=="Hero");
 for(auto point:std::vector<dx::Point>{{0,0},{40,170},{-40,-170},{85,179.9}}){auto restored=dx::geographic(dx::mercator(point));CHECK(std::abs(restored.lat-point.lat)<1e-6&&std::abs(restored.lon-point.lon)<1e-6);}
 CHECK(std::isfinite(dx::mercator({90,0}).lat));
 std::vector<std::vector<dx::Point>> country={{{-5,-10},{-5,10},{5,10},{5,-10},{-5,-10}}};CHECK(dx::insidePolygon({0,0},country));CHECK(dx::labelFits({0,0},10,4,country));CHECK(!dx::labelFits({0,0},25,4,country));CHECK(!dx::labelFits({0,0},10,12,country));auto center=dx::countryCenter(country[0],{1,1});CHECK(std::abs(center.lat)<1e-6&&std::abs(center.lon)<1e-6);
 country.push_back({{-1,-1},{-1,1},{1,1},{1,-1},{-1,-1}});CHECK(!dx::insidePolygon({0,0},country));CHECK(!dx::labelFits({0,0},10,4,country));
 CHECK(!dx::gridPoint("RR73"));CHECK(!dx::gridPoint("A"));CHECK(!dx::gridPoint("ZZ00"));CHECK(dx::gridPoint("KM69"));
 CHECK(std::round(dx::distanceBearing(*dx::gridPoint("KM69"),*dx::gridPoint("KN07")).first)==1312);
 // ARRL grid-center calculator: KM69 -> NO35 = 4296.5 km, using R=6371.14.
 auto ankara=*dx::gridPoint("KM69"),rk9ue=*dx::gridPoint("NO35");CHECK(ankara.lat==39.5&&ankara.lon==33&&rk9ue.lat==55.5&&rk9ue.lon==87);
 auto siberia=dx::distanceBearing(ankara,rk9ue);CHECK(std::abs(siberia.first-4296.401573)<.001&&std::abs(siberia.second-47.211998)<.001);
 CHECK(std::abs(dx::distanceBearing(rk9ue,ankara).first-siberia.first)<1e-6);
 CHECK(dx::distanceBearing(ankara,ankara).first==0);CHECK(std::abs(dx::distanceBearing({0,179},{0,-179}).first-222.39016)<.01);
 auto home=*dx::gridPoint("KM69"),boston=*dx::gridPoint("FN42");auto distance=dx::distanceBearing(home,boston);CHECK(distance.first>7500&&distance.first<9000);CHECK(std::isfinite(distance.second));
 {
  dx::Store unknownHome(":memory:");dx::Decode heard;heard.utc=1800000000;heard.dial=7074000;heard.mode="FT8";heard.message="CQ K1ABC FN42";heard.km=1234;heard.bearing=90;
  CHECK(unknownHome.insert(heard,{NAN,NAN}));CHECK(heard.position&&!heard.km&&!heard.bearing&&!heard.distanceRecord);CHECK(unknownHome.stats(heard.utc).farthest==0);
  unknownHome.rebaseDistances(home);CHECK(unknownHome.rows(dx::Query{}).front().decode.km);unknownHome.rebaseDistances({NAN,NAN});CHECK(!unknownHome.rows(dx::Query{}).front().decode.km);CHECK(unknownHome.stats(heard.utc).farthest==0);
 }
 auto dateline=dx::greatCircle({40,170},{40,-170});CHECK(dateline.size()==129);for(auto p:dateline)CHECK(std::isfinite(p.lat)&&std::isfinite(p.lon));
 for(auto pair:std::vector<std::pair<dx::Point,dx::Point>>{{{90,0},{-90,0}},{{0,0},{0,180}},{{42,1},{42,1}}})for(auto p:dx::greatCircle(pair.first,pair.second))CHECK(std::isfinite(p.lat)&&std::isfinite(p.lon));
 auto m=dx::meaning("K1ABC TA1XYZ RR73");CHECK(m.transmitter=="TA1XYZ"&&m.called=="K1ABC"&&m.grid.empty()&&m.type=="RR73");CHECK(dx::meaning("CQ DX K1ABC FN42").transmitter=="K1ABC");CHECK(dx::meaning("HELLO WORLD").transmitter.empty());CHECK(dx::meaning("K1ABC <1:123> -10").transmitter=="<1:123>");
 CHECK(dx::matches("ta1xyz","K*,TA*"));CHECK(!dx::matches("K1ABC","K2*"));dx::Decode d;d.utc=1800000000;d.dial=7074000;d.mode="FT8";d.audio=1000;d.snr=-15;d.dt=.3;d.message="CQ K1ABC FN42";d.identity=dx::meaning(d.message);
 dx::Filter f;f.call="K*";CHECK(f.accepts(d));f.excludeCall="K1*";CHECK(!f.accepts(d));f={};f.maxDt=.2;CHECK(!f.accepts(d));f.maxDt=30;CHECK(f.accepts(d));
 f={};f.messageKind=dx::MessageKind::GeneralCall;CHECK(f.accepts(d));CHECK(dx::Filter::fromJson(f.json()).messageKind==f.messageKind);
 for(const auto& message:std::vector<std::string>{"CQ DX K1ABC FN42","QRZ K1ABC FN42"})CHECK(dx::messageKind(dx::meaning(message))==dx::MessageKind::GeneralCall);
 for(const auto& message:std::vector<std::string>{"K2ABC K1ABC FN42","K2ABC K1ABC -10","K2ABC K1ABC R-10","K2ABC K1ABC RRR","K2ABC K1ABC RR73","K2ABC <1:123> 73"})CHECK(dx::messageKind(dx::meaning(message))==dx::MessageKind::Exchange);
 CHECK(dx::messageKind(dx::meaning("HELLO WORLD"))==dx::MessageKind::Other);CHECK(dx::messageKind(dx::meaning("DE K1ABC FN42"))==dx::MessageKind::Other);
 CHECK(dx::Filter::fromJson(dx::Json{{"cqOnly",true}}).cqOnly);
 auto west=d;west.km=1000;west.bearing=359;auto east=west;east.bearing=1;auto bearing=dx::activityBearing({west,east});CHECK(bearing.degrees&&std::min(*bearing.degrees,360-*bearing.degrees)<1e-8&&bearing.stations==2);east.bearing=179;CHECK(!dx::activityBearing({west,east}).degrees);CHECK(dx::activityBearing({}).stations==0);CHECK(dx::compassDirection(225)=="Southwest"&&dx::compassDirection(359)=="North");CHECK(dx::arrivalOpacity(0)==0&&dx::arrivalOpacity(.45)>.99&&dx::arrivalOpacity(6)<dx::arrivalOpacity(3)&&dx::arrivalOpacity(3)>0&&dx::arrivalOpacity(9)==0);
 auto mesh=dx::countryMesh(country,false);CHECK(mesh.indices.size()%3==0&&!mesh.indices.empty());double filled=0;
 for(size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.points.at(mesh.indices[i]),b=mesh.points.at(mesh.indices[i+1]),c=mesh.points.at(mesh.indices[i+2]);filled+=std::abs((b.lon-a.lon)*(c.lat-a.lat)-(c.lon-a.lon)*(b.lat-a.lat))/2;CHECK(dx::insidePolygon({(a.lat+b.lat+c.lat)/3,(a.lon+b.lon+c.lon)/3},country));}CHECK(std::abs(filled-196)<1e-8);
 auto seam=dx::countryMesh({{{-1,179},{-1,-179},{1,-179},{1,179}}},false);CHECK(seam.max.lon-seam.min.lon==2&&seam.indices.size()==6);
 CHECK(dx::countryName("Fed. Rep. of Germany")=="Germany"&&dx::countryName("France")=="France");CHECK(dx::Filter::fromJson({{"country","Fed. Rep. of Germany,France"},{"excludeCountry","Fed. Rep. of Germany"}}).country=="Germany,France");CHECK(dx::Filter::fromJson({{"excludeCountry","Fed. Rep. of Germany"}}).excludeCountry=="Germany");
 CHECK(dx::countryOutlineKey("Fed. Rep. of Germany")==dx::countryOutlineKey("Germany"));CHECK(dx::countryOutlineKey("Asiatic Russia")==dx::countryOutlineKey("Russia"));CHECK(dx::countryOutlineKey("Guernsey")!=dx::countryOutlineKey("United Kingdom"));
 auto channels=dx::eligible(7060000,1024000,dx::presets());CHECK(channels.size()==2);CHECK(dx::eligible(7074000,3000,dx::presets()).empty());
 auto path=std::filesystem::temp_directory_path()/("dx-test-"+std::to_string(getpid())+".sqlite3");{
 dx::Store store(path);CHECK(store.insert(d,home));CHECK(!store.insert(d,home));CHECK(d.km&&d.gridSource=="received"&&d.distanceRecord);CHECK(dx::Decode::fromJson(d.json()).distanceRecord);auto legacy=d;legacy.source="legacy";CHECK(!store.insert(legacy,home));dx::Decode report=d;report.utc+=15;report.message="K2ABC K1ABC -10";CHECK(store.insert(report,home));CHECK(report.gridSource=="cached"&&report.position);auto stats=store.stats(report.utc);CHECK(stats.stations==1&&stats.messages==2&&stats.recentStations==1);dx::Query q;auto rows=store.rows(q);CHECK(rows.size()==1&&rows[0].count==2);q.messages=true;CHECK(store.rows(q).size()==2);q.filter.excludeCall="K1*";CHECK(store.rows(q).empty());CHECK(store.history("K1ABC",7074000,"FT8").size()==2);
 }
 auto uniquePath=std::filesystem::temp_directory_path()/("dx-unique-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(uniquePath);auto live=d;live.message="CQ F6ARS JN38";live.dial=7047500;live.mode="FT4";CHECK(store.insert(live,home));auto imported=live;imported.dial=7048000;imported.source="legacy";imported.snr=-14;CHECK(store.insert(imported,home));dx::Query q;q.uniqueCalls=true;auto rows=store.rows(q);CHECK(rows.size()==1&&rows.front().decode.dial==7048000&&rows.front().decode.snr==-14&&rows.front().count==2);
  auto latest=live;latest.utc+=15;latest.dial=14074000;latest.mode="FT8";latest.message="G1ABC F6ARS -07";latest.snr=-5;CHECK(store.insert(latest,home));rows=store.rows(q);CHECK(rows.size()==1&&rows.front().decode.message==latest.message&&rows.front().decode.mode=="FT8"&&rows.front().decode.snr==-5&&rows.front().count==3&&rows.front().stationFirstHeardUtc==live.utc);q.filter.band="40m";rows=store.rows(q);CHECK(rows.size()==1&&rows.front().decode.dial==7048000&&rows.front().decode.message==live.message&&rows.front().count==2);CHECK(store.stats(latest.utc).messages==3);
 }std::filesystem::remove(uniquePath);std::filesystem::remove(uniquePath.string()+"-wal");std::filesystem::remove(uniquePath.string()+"-shm");
 auto germanyPath=std::filesystem::temp_directory_path()/("dx-germany-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(germanyPath);auto german=d;german.message="CQ DL1ABC JO62";CHECK(store.insert(german,home,[](const std::string&){return std::make_pair("Fed. Rep. of Germany","EU");}));CHECK(german.country=="Germany"&&german.json()["country"]=="Germany");auto old=german.json();old["country"]="Fed. Rep. of Germany";CHECK(dx::Decode::fromJson(old).country=="Germany");store.setSetting("country_names",0);store.setSetting("farthest_country","Fed. Rep. of Germany");
 }
 {sqlite3* db=nullptr;CHECK(sqlite3_open(germanyPath.c_str(),&db)==SQLITE_OK);CHECK(sqlite3_exec(db,"UPDATE decodes SET country='Fed. Rep. of Germany',payload=json_set(payload,'$.country','Fed. Rep. of Germany');UPDATE stations SET payload=json_set(payload,'$.country','Fed. Rep. of Germany');UPDATE heard SET value='Fed. Rep. of Germany' WHERE kind='country' AND value='Germany';",nullptr,nullptr,nullptr)==SQLITE_OK);sqlite3_close(db);}
 {dx::Store store(germanyPath);dx::Query q;q.filter.country="Germany";auto rows=store.rows(q);CHECK(rows.size()==1&&rows[0].decode.country=="Germany");CHECK(store.stats(d.utc).countries==1&&store.stats(d.utc).farthestCountry=="Germany");CHECK(store.getSetting("farthest_country","")=="Germany");CHECK(store.logRows(0).front().second.country=="Germany");CHECK(store.rows(dx::Query{}).front().decode.country=="Germany");}
 std::filesystem::remove(germanyPath);std::filesystem::remove(germanyPath.string()+"-wal");std::filesystem::remove(germanyPath.string()+"-shm");
 auto cachePath=std::filesystem::temp_directory_path()/("dx-cache-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(cachePath);auto reception=[&](int seconds,const std::string& message){dx::Decode heard;heard.utc=d.utc+seconds;heard.dial=d.dial;heard.mode=d.mode;heard.audio=1000;heard.snr=-12;heard.message=message;if(!store.insert(heard,home))throw std::runtime_error("Reception fixture was not inserted");return heard;};
  auto grid=reception(0,"CQ K1ABC FN42");auto first=reception(15,"K2ABC K1ABC -10");auto second=reception(30,"K2ABC K1ABC RR73");CHECK(first.gridSource=="cached"&&second.gridSource=="cached"&&second.km&&second.gridUtc==grid.utc);
  auto expired=reception(21601,"K2ABC K1ABC 73");CHECK(!expired.km&&expired.gridSource.empty());
  auto unknown=reception(21615,"CQ LZ936BA");CHECK(!unknown.km&&!unknown.position);
  CHECK(dx::latestHeardSummary(unknown).find("distance unknown | -12 dB SNR")!=std::string::npos);CHECK(dx::latestHeardSummary(grid).find("km | -12 dB SNR")!=std::string::npos);
 }
 std::filesystem::remove(cachePath);std::filesystem::remove(cachePath.string()+"-wal");std::filesystem::remove(cachePath.string()+"-shm");
 auto newestCountriesPath=std::filesystem::temp_directory_path()/("dx-newest-countries-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(newestCountriesPath);std::vector<dx::Decode> discoveries;
  for(int n=0;n<5;n++){
   auto first=d;first.utc+=n*15;first.message="CQ K"+std::to_string(n+1)+"ABC FN42";first.country="Country "+std::to_string(n);CHECK(store.insert(first,home));discoveries.push_back(first);
   auto stats=store.sessionStats(first.utc,0,d.utc);CHECK(stats.countries==n+1&&stats.newestCountries.size()==size_t(std::min(3,n+1)));
   for(size_t i=0;i<stats.newestCountries.size();i++)CHECK(stats.newestCountries[i]==discoveries[size_t(n)-i].country);
  }
  auto updated=discoveries.back();updated.utc+=90;updated.message="K8ABC K5ABC -10";CHECK(store.insert(updated,home));dx::Query newest;newest.uniqueCalls=true;newest.filter.call="K5ABC";auto shown=store.rows(newest);auto updatedStats=store.sessionStats(updated.utc,0,d.utc);CHECK(shown.size()==1&&shown.front().decode.key()==updated.key());CHECK(updatedStats.isNewestCountry(shown.front().decode.country));
  auto repeat=discoveries.front();repeat.utc+=120;CHECK(store.insert(repeat,home));auto stats=store.sessionStats(repeat.utc,0,d.utc);CHECK(stats.newestCountries==std::vector<std::string>({discoveries[4].country,discoveries[3].country,discoveries[2].country}));
  CHECK(!stats.isNewestCountry(discoveries[0].country)&&!stats.isNewestCountry(discoveries[1].country)&&stats.isNewestCountry(discoveries[2].country));
  auto secondStation=repeat;secondStation.utc+=15;secondStation.message="CQ K6ABC FN42";CHECK(store.insert(secondStation,home));CHECK(store.sessionStats(secondStation.utc,0,d.utc).newestCountries==stats.newestCountries);
  auto unknown=repeat;unknown.utc+=30;unknown.country.clear();unknown.message="CQ K7ABC FN42";CHECK(store.insert(unknown,home));CHECK(store.sessionStats(unknown.utc,0,d.utc).newestCountries==stats.newestCountries);
  auto cutoff=store.lastReceptionId();CHECK(store.sessionStats(unknown.utc,cutoff,unknown.utc).newestCountries.empty());repeat.utc+=60;CHECK(store.insert(repeat,home));CHECK(store.sessionStats(repeat.utc,cutoff,unknown.utc).newestCountries==std::vector<std::string>({repeat.country}));
 }
 std::filesystem::remove(newestCountriesPath);std::filesystem::remove(newestCountriesPath.string()+"-wal");std::filesystem::remove(newestCountriesPath.string()+"-shm");
 auto sessionPath=std::filesystem::temp_directory_path()/("dx-session-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(sessionPath);auto country=[](const std::string& call){return std::make_pair(call[0]=='G'?"United Kingdom":"United States","NA");};auto old=d;CHECK(store.insert(old,home,country));auto cutoff=store.lastReceptionId();auto blank=store.sessionStats(d.utc,cutoff,d.utc);CHECK(blank.messages==0&&blank.stations==0&&blank.countries==0&&blank.recentStations==0&&blank.farthest==0&&blank.latest.empty());
  auto heard=d;heard.utc+=15;CHECK(store.insert(heard,home,country));auto again=heard;again.utc+=15;again.message="K2ABC K1ABC RR73";CHECK(store.insert(again,home,country));auto british=d;british.utc+=30;british.message="CQ G1ABC IO91";CHECK(store.insert(british,home,country));auto stats=store.sessionStats(british.utc,cutoff,d.utc);CHECK(stats.messages==3&&stats.stations==2&&stats.countries==2&&stats.recentStations==2&&stats.latest.size()==2&&stats.farthest>7500&&stats.farthestCountry=="United States");
  CHECK(stats.farthestCall=="K1ABC"&&stats.newestCountries.size()==2);CHECK(stats.isNewestCountry(heard.country));CHECK(stats.isNewestCountry(again.country));
  cutoff=store.lastReceptionId();CHECK(store.sessionStats(british.utc,cutoff,british.utc).messages==0);auto sameSlot=british;sameSlot.audio+=1;CHECK(store.insert(sameSlot,home,country));CHECK(store.sessionStats(british.utc,cutoff,british.utc).messages==1);
  auto historical=d;historical.utc-=1000;historical.audio+=2;historical.source="legacy";CHECK(store.insert(historical,home,country));CHECK(store.sessionStats(british.utc,cutoff,british.utc).messages==1);CHECK(store.stats(british.utc).messages==6);
  dx::Query q;q.afterRowId=cutoff;q.filter.call="G*";CHECK(store.rows(q).size()==1);int64_t count=0;store.rows(q,0,0,&count);CHECK(count==1);
 }
 std::filesystem::remove(sessionPath);std::filesystem::remove(sessionPath.string()+"-wal");std::filesystem::remove(sessionPath.string()+"-shm");
 auto kindsPath=std::filesystem::temp_directory_path()/("dx-kinds-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(kindsPath);std::vector<dx::Decode> decodes;int delta=0;
  for(const auto& message:std::vector<std::string>{"CQ K1ABC FN42","CQ DX K1ABC FN42","QRZ K1ABC FN42","K2ABC K1ABC -10","K2ABC K1ABC RR73","K2ABC <1:123> 73","HELLO WORLD","DE K1ABC FN42"}){auto entry=d;entry.utc+=delta++*15;entry.message=message;CHECK(store.insert(entry,home));decodes.push_back(entry);}
  for(auto kind:{dx::MessageKind::GeneralCall,dx::MessageKind::Exchange,dx::MessageKind::Other})for(bool grouped:{false,true}){
   dx::Query query;query.messages=!grouped;query.filter.messageKind=kind;auto rows=store.rows(query);CHECK(!rows.empty());int64_t count=0;store.rows(query,0,0,&count);CHECK(count==int64_t(rows.size()));
   for(const auto& row:rows)CHECK(query.filter.accepts(row.decode)&&dx::messageKind(row.decode.identity)==kind);
   if(!grouped){size_t expected=0;for(const auto& entry:decodes)if(query.filter.accepts(entry))expected++;CHECK(rows.size()==expected);}
  }
 }
 std::filesystem::remove(kindsPath);std::filesystem::remove(kindsPath.string()+"-wal");std::filesystem::remove(kindsPath.string()+"-shm");
 auto exchangePath=std::filesystem::temp_directory_path()/("dx-exchanges-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(exchangePath);auto insert=[&](int offset,const std::string& text){auto heard=d;heard.utc=1800000000+offset;heard.message=text;return store.insert(heard,home);};
  CHECK(insert(0,"CQ K1ABC FN42")&&insert(1,"CQ K2ABC IO91")&&insert(2,"CQ G1ABC JN58")&&insert(3,"CQ G2ABC KN34"));
  CHECK(insert(8,"G2ABC G1ABC -10")&&insert(10,"K2ABC K1ABC -12")&&insert(11,"K1ABC K2ABC R-12")&&insert(12,"K2ABC K1ABC RR73"));
  dx::Query q;q.until=1800000300;auto paths=store.exchangePaths(q);CHECK(paths.size()==2&&paths[0].transmitter=="K1ABC"&&paths[0].called=="K2ABC"&&paths[1].transmitter=="G1ABC");
  CHECK(std::abs(paths[0].points.front().lat-dx::gridPoint("FN42")->lat)<1e-6&&std::abs(paths[0].points.back().lon-dx::gridPoint("IO91")->lon)<1e-6);
  for(int i=0;i<110;i++)CHECK(insert(100+i,"CQ W1ABC FN31"));CHECK(store.exchangePaths(q).size()==2);
  CHECK(insert(220,"N2ABC N1ABC -10"));CHECK(store.exchangePaths(q).size()==2);
  CHECK(insert(230,"CQ N1ABC EM10")&&insert(240,"CQ N2ABC JO89"));paths=store.exchangePaths(q);CHECK(paths.size()==3&&paths[0].transmitter=="N1ABC"&&paths[0].called=="N2ABC");
  q.pathCount=1;CHECK(store.exchangePaths(q).size()==1);q.pathCount=0;CHECK(store.exchangePaths(q).empty());q.pathCount=3;q.filter.messageKind=dx::MessageKind::GeneralCall;CHECK(store.exchangePaths(q).empty());q.filter={};q.since=1800000301;CHECK(store.exchangePaths(q).empty());
 }
 std::filesystem::remove(exchangePath);std::filesystem::remove(exchangePath.string()+"-wal");std::filesystem::remove(exchangePath.string()+"-shm");
 auto firstHeardPath=std::filesystem::temp_directory_path()/("dx-first-heard-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(firstHeardPath);auto first=d;first.utc=1800000000;first.snr=-20;CHECK(store.insert(first,home));
  auto repeat=first;repeat.utc+=15;repeat.snr=-4;repeat.audio+=100;CHECK(store.insert(repeat,home));
  dx::Query q;auto rows=store.rows(q);CHECK(rows.size()==1&&rows[0].decode.utc==repeat.utc&&rows[0].firstHeardUtc==first.utc);
  q.filter.minSnr=-5;CHECK(store.rows(q).front().firstHeardUtc==first.utc);q.filter={};
  auto changed=repeat;changed.utc+=15;changed.message="K2ABC K1ABC -10";CHECK(store.insert(changed,home));CHECK(store.rows(q).front().firstHeardUtc==changed.utc&&store.rows(q).front().stationFirstHeardUtc==first.utc);
  auto changedRepeat=changed;changedRepeat.utc+=15;CHECK(store.insert(changedRepeat,home));CHECK(store.rows(q).front().firstHeardUtc==changed.utc);
  repeat.utc+=45;CHECK(store.insert(repeat,home));CHECK(store.rows(q).front().firstHeardUtc==first.utc);
  q.messages=true;rows=store.rows(q);CHECK(rows.size()==5);for(const auto& row:rows)CHECK(row.firstHeardUtc==(row.decode.message==first.message?first.utc:changed.utc));
  q.sort=16;q.descending=false;CHECK(store.rows(q).front().firstHeardUtc==first.utc);q.descending=true;CHECK(store.rows(q,1,1).front().firstHeardUtc==changed.utc);
  q={};q.since=repeat.utc;CHECK(store.rows(q).front().firstHeardUtc==first.utc&&store.rows(q).front().stationFirstHeardUtc==first.utc);
  q.firstSince=repeat.utc;CHECK(store.rows(q).front().firstHeardUtc==repeat.utc&&store.rows(q).front().stationFirstHeardUtc==repeat.utc);
  auto baseline=store.lastReceptionId();auto sameSlot=repeat;sameSlot.audio++;CHECK(store.insert(sameSlot,home));q={};q.afterRowId=baseline;CHECK(store.rows(q).front().firstHeardUtc==first.utc&&store.rows(q).front().stationFirstHeardUtc==first.utc);
  q.firstAfterId=baseline;CHECK(store.rows(q).front().firstHeardUtc==sameSlot.utc&&store.rows(q).front().stationFirstHeardUtc==sameSlot.utc);
  auto otherBand=first;otherBand.dial+=7000000;otherBand.utc+=75;CHECK(store.insert(otherBand,home));auto otherMode=first;otherMode.mode="FT4";otherMode.utc+=90;CHECK(store.insert(otherMode,home));q={};rows=store.rows(q);CHECK(rows.size()==3);for(const auto& row:rows)CHECK(row.firstHeardUtc==first.utc&&row.stationFirstHeardUtc==first.utc);
  q.uniqueCalls=true;CHECK(store.rows(q).front().firstHeardUtc==first.utc);q={};int64_t count=0;store.rows(q,0,0,&count);CHECK(count==3);
  auto unique=store.uniqueMessages(first,q);CHECK(unique.size()==2&&unique[0].message==first.message&&unique[0].utc==sameSlot.utc&&unique[1].message==changed.message&&unique[1].utc==changedRepeat.utc);
  for(int i=0;i<6;i++){auto report=first;report.utc+=120+i*15;report.message="K2ABC K1ABC -"+std::to_string(20+i);CHECK(store.insert(report,home));}unique=store.uniqueMessages(first,q);CHECK(unique.size()==5&&unique.front().message=="K2ABC K1ABC -25"&&unique.back().message=="K2ABC K1ABC -21");q.since=unique.front().utc;CHECK(store.uniqueMessages(first,q).size()==1);q.since++;CHECK(store.uniqueMessages(first,q).empty());
 }
 {dx::Store store(firstHeardPath);dx::Query q;CHECK(store.rows(q).back().firstHeardUtc==1800000000&&store.rows(q).front().stationFirstHeardUtc==1800000000);}
 std::filesystem::remove(firstHeardPath);std::filesystem::remove(firstHeardPath.string()+"-wal");std::filesystem::remove(firstHeardPath.string()+"-shm");
 auto sortingPath=std::filesystem::temp_directory_path()/("dx-sort-"+std::to_string(getpid())+".sqlite3");{
  dx::Store store(sortingPath);
  auto first=d;first.message="CQ K1AAA FN42";first.utc=1800000000;first.snr=-20;first.audio=2000;first.dt=.1;CHECK(store.insert(first,home,[](const std::string&){return std::make_pair("Z Country","NA");}));
  auto second=first;second.message="CQ G1BBB IO91";second.utc+=15;second.dial=14074000;second.mode="FT4";second.snr=-5;second.audio=500;second.dt=.7;CHECK(store.insert(second,home,[](const std::string&){return std::make_pair("A Country","EU");}));
  auto last=first;last.utc+=30;last.message="G1BBB K1AAA -10";CHECK(store.insert(last,home,[](const std::string&){return std::make_pair("Z Country","NA");}));
  dx::Query q;auto newest=store.rows(q);CHECK(newest.size()==2&&newest[0].decode.utc==last.utc);
  for(bool messages:{false,true})for(bool filtered:{false,true})for(int sort=0;sort<18;sort++){
   q={};q.messages=messages;q.sort=sort;if(filtered)q.filter.call="*";q.descending=false;auto ascending=store.rows(q);q.descending=true;auto descending=store.rows(q);
   CHECK(ascending.size()==(messages?3:2)&&descending.size()==ascending.size());
   if(sort==2)CHECK(ascending.front().decode.country=="A Country"&&descending.front().decode.country=="Z Country");
   if(sort==4)CHECK(ascending.front().decode.snr==-20&&descending.front().decode.snr==-5);
   if(sort==7)CHECK(dx::bandName(ascending.front().decode.dial)=="20m"&&dx::bandName(descending.front().decode.dial)=="40m");
   if(sort==9)CHECK(ascending.front().decode.audio==500&&descending.front().decode.audio==2000);
   if(sort==10)CHECK(ascending.front().decode.dt==.1&&descending.front().decode.dt==.7);
   if(sort==12&&!messages)CHECK(ascending.front().count==1&&descending.front().count==2);
   if(sort==17)CHECK(ascending.front().stationFirstHeardUtc==first.utc&&descending.front().stationFirstHeardUtc==second.utc);
  }
  q={};q.messages=true;q.sort=10;auto unknown=second;unknown.utc+=60;unknown.dt.reset();CHECK(store.insert(unknown,home));q.descending=false;CHECK(!store.rows(q).back().decode.dt);q.descending=true;CHECK(!store.rows(q).back().decode.dt);
 }
 std::filesystem::remove(sortingPath);std::filesystem::remove(sortingPath.string()+"-wal");std::filesystem::remove(sortingPath.string()+"-shm");
 auto collision=dx::callHash("K1ABC",10);CHECK(collision<1024);
 auto alert=d;alert.km=8000;alert.gridSource="received";alert.firstCall=true;alert.source="live";alert.utc=1800000000;dx::AlertPolicy policy;CHECK(!policy.sound);dx::Alerts alerts;alerts.baseline({});auto event=alerts.evaluate({alert},policy,alert.utc);CHECK(event&&!event->sound);CHECK(!alerts.evaluate({alert},policy,alert.utc+60));alert.source="legacy";alert.utc+=100;CHECK(!alerts.evaluate({alert},policy,alert.utc));
 alerts.baseline({{alert,1}});alert.source="live";alert.utc+=100;CHECK(!alerts.evaluate({alert},policy,alert.utc));alert.km=9000;alert.utc+=100;CHECK(alerts.evaluate({alert},policy,alert.utc));
 std::filesystem::remove(path);std::filesystem::remove(path.string()+"-wal");std::filesystem::remove(path.string()+"-shm");
 dx::SlotCapture capture;std::vector<float> samples(1200,.1f);int jobs=0;for(int n=0;n<150;n++){auto j=capture.feed(samples.data(),samples.size(),15000+n*100+100,7074000,"FT8");if(j){++jobs;CHECK(j->pcm.size()==180000&&j->utcMs==15000);}}CHECK(jobs==1);
 capture.reset();for(int n=0;n<149;n++)CHECK(!capture.feed(samples.data(),samples.size(),30050+n*100+100,7074000,"FT8"));
 auto summer=dx::sunPosition(1782043200);CHECK(std::abs(summer.lat)>22&&std::abs(summer.lat)<24);
 std::cout<<"Model, storage, geometry, filters, provenance, capture checks passed\n";return 0;
}
