// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <limits>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
namespace {
using Polygon=std::vector<std::vector<dx::Point>>;
void world(const std::filesystem::path& file,const std::vector<Polygon>& polygons){
 dx::Json coordinates=dx::Json::array();for(const auto& polygon:polygons){dx::Json rings=dx::Json::array();for(const auto& ring:polygon){dx::Json points=dx::Json::array();for(auto p:ring)points.push_back({p.lon,p.lat});rings.push_back(points);}coordinates.push_back(rings);}
 std::ofstream out(file);out<<dx::Json{{"features",dx::Json::array({{{"properties",{{"NAME_EN","Testland"}}},{"geometry",{{"type","MultiPolygon"},{"coordinates",coordinates}}}}})}};
}
double brute(dx::Point home,const std::vector<Polygon>& polygons,int minX,int maxX,int minY,int maxY){
 double best=std::numeric_limits<double>::infinity();for(int y=minY;y<=maxY;++y)for(int x=minX;x<=maxX;++x){dx::Point point{(y+.5)/24.-90,(x+.5)/12.-180};for(const auto& polygon:polygons)if(dx::insidePolygon(point,polygon)){best=std::min(best,dx::distanceBearing(home,point).first);break;}}return best;
}
}
int main(int argc,char** argv){
 if(argc!=3)return 2;auto pattern=(std::filesystem::temp_directory_path()/"dx-country-distance-XXXXXX").string();CHECK(mkdtemp(pattern.data()));std::filesystem::path folder=pattern;TestFolderCleanup cleanup(folder);
 auto file=folder/"geometry.json";dx::LocationCheck geometry;
 std::vector<Polygon> square{{{{-1,9},{-1,11},{1,11},{1,9},{-1,9}}}};
 world(file,square);CHECK(geometry.load(file));auto estimate=geometry.nearestCountryGrid("Testland",{0,0});CHECK(estimate&&estimate->grid.size()==6);CHECK(std::abs(estimate->km-brute({0,0},square,2268,2292,2136,2184))<1e-6);CHECK(dx::insidePolygon(*dx::gridPoint(estimate->grid),square[0]));
 auto hole=square;hole[0].push_back({{-.3,9.4},{-.3,10.6},{.3,10.6},{.3,9.4},{-.3,9.4}});world(file,hole);CHECK(geometry.load(file));estimate=geometry.nearestCountryGrid("Testland",{0,10});CHECK(estimate&&std::abs(estimate->km-brute({0,10},hole,2268,2292,2136,2184))<1e-6);
 std::vector<Polygon> islands{{{{-1,30},{-1,32},{1,32},{1,30},{-1,30}}},{{{-.2,5},{-.2,5.3},{.2,5.3},{.2,5},{-.2,5}}}};world(file,islands);CHECK(geometry.load(file));estimate=geometry.nearestCountryGrid("Testland",{0,0});CHECK(estimate&&estimate->km<600&&std::abs(estimate->km-brute({0,0},islands,2220,2544,2136,2184))<1e-6);
 std::vector<Polygon> dateline{{{{79,179},{79,-179},{81,-179},{81,179},{79,179}}}};world(file,dateline);CHECK(geometry.load(file));estimate=geometry.nearestCountryGrid("Testland",{80,178});CHECK(estimate&&estimate->km<25&&std::abs(estimate->km-brute({80,178},dateline,0,4319,4056,4104))<1e-6);
 CHECK(!geometry.nearestCountryGrid("Unknown",{0,0})&&!geometry.nearestCountryGrid("Testland",{NAN,NAN}));
 std::vector<Polygon> wide{{{{30,10},{30,50},{35,50},{35,10},{30,10}}}};world(file,wide);CHECK(geometry.load(file));estimate=geometry.nearestCountryGrid("Testland",{0,30});CHECK(estimate&&std::abs(estimate->km-brute({0,30},wide,2280,2760,2880,3000))<1e-6);
 CHECK(geometry.load(argv[1]));auto home=*dx::gridPoint("KM69");for(const auto& country:{"Germany","Japan","United States"}){estimate=geometry.nearestCountryGrid(country,home);CHECK(estimate&&estimate->km>0&&dx::gridPoint(estimate->grid));}
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{
  {"European Russia","Russia"},{"Asiatic Russia","Russia"},{"Asian Russia","Russia"},
  {"European Turkey","Turkey"},{"Asiatic Turkey","Turkey"},{"Asian Turkey","Turkey"},
  {"Turkiye","Turkey"},{"Türkiye","Turkey"},{"Republic of Türkiye","Turkey"},
  {"West Malaysia","Malaysia"},{"East Malaysia","Malaysia"},
  {"Dem. Rep. of the Congo","Democratic Republic of the Congo"},
  {"Bosnia-Herzegovina","Bosnia and Herzegovina"},{"Trinidad & Tobago","Trinidad and Tobago"},
  {"DPR of Korea","North Korea"},{"DPR Korea","North Korea"},{"Republic of Korea","South Korea"},
  {"Cote d'Ivoire","Ivory Coast"},{"Côte d'Ivoire","Ivory Coast"},
  {"Fed. Rep. of Germany","Germany"},{"United States","United States of America"},
  {"Burma","Myanmar"},{"Swaziland","Eswatini"},{"Macedonia","North Macedonia"},
  {"  european RUSSIA. ","Russia"}}){
  auto alias=geometry.nearestCountryGrid(pair.first,home),canonical=geometry.nearestCountryGrid(pair.second,home);
  CHECK(alias&&canonical&&alias->grid==canonical->grid&&alias->km==canonical->km);
 }
 for(const auto& unknown:{"Congo","European Unknown","England","Scotland","Kaliningrad","Canary Islands","Sov Mil Order of Malta"})CHECK(!geometry.nearestCountryGrid(unknown,home));
 estimate=geometry.nearestCountryGrid("United States",{35,140});CHECK(estimate&&dx::gridPoint(estimate->grid)->lon>-130&&dx::gridPoint(estimate->grid)->lat>24);
 estimate=geometry.nearestCountryGrid("France",{-3,-52});CHECK(estimate&&dx::gridPoint(estimate->grid)->lat>40);
 auto begin=dx::monotonicMs();for(int i=0;i<10000;i++)CHECK(geometry.nearestCountryGrid("Germany",home));CHECK(dx::monotonicMs()-begin<3000);
 auto country=[](const std::string& call){return std::make_pair(call.rfind("DL",0)==0?"Germany":call.rfind("JH",0)==0?"Japan":"","EU");};
 std::vector<dx::Json> original;std::filesystem::create_directory(folder/"profile");std::filesystem::copy_file(argv[1],folder/"world.geojson");std::filesystem::create_symlink(std::filesystem::absolute(argv[2]),folder/"dx_mock_valid_small");
 {
  dx::Store store(folder/"profile/history.sqlite3");CHECK(store.loadCountryAreas(argv[1]));store.countryDistances(true,home);int64_t utc=time(nullptr)-30;
  for(const auto& message:{"JH1DSX DL1ABC -03","CQ DL2ABC JO33","JH1DSX DL2ABC RR73","K1ABC JH1ABC -09","K1ABC ZZ1ABC -09"}){dx::Decode d;d.utc=utc++;d.slotMs=d.utc*1000;d.mode="FT8";d.dial=14074000;d.message=message;CHECK(store.insert(d,home,country));CHECK(d.countryDistanceGrid.empty());}
  store.rebaseDistances(home);for(const auto& [id,d]:store.logRows(0))original.push_back(d.json());
  dx::Query q;q.messages=true;q.sort=3;q.descending=false;auto rows=store.rows(q);CHECK(rows.size()==5);
  double previous=-1;int estimated=0;for(const auto& row:rows){const auto& d=row.decode;if(d.km){CHECK(*d.km>=previous);previous=*d.km;}if(!d.countryDistanceGrid.empty()){estimated++;CHECK(d.identity.grid.empty()&&!d.position&&!d.bearing&&!d.distanceRecord);CHECK(dx::distancePrefix(d)=="[C] ");CHECK(dx::Decode::fromJson(d.json()).countryDistanceGrid==d.countryDistanceGrid);}}
  CHECK(estimated==2&&!rows.back().decode.km);auto expected=geometry.nearestCountryGrid("Germany",home)->km;
  dx::Query cached;cached.sort=3;cached.descending=false;auto cachedRows=store.rows(cached);CHECK(cachedRows.size()==4&&cachedRows.front().decode.km&&std::abs(*cachedRows.front().decode.km-expected)<1e-6);
  q.filter.minKm=expected-.01;q.filter.maxKm=expected+.01;CHECK(store.rows(q).size()==1);int64_t count=0;store.rows(q,0,0,&count);CHECK(count==1);
  q.filter={};q.filter.columnRanges[size_t(dx::ColumnMetric::Distance)]={expected-.01,expected+.01};CHECK(store.rows(q).size()==1);
  q.filter={};q.uniqueCalls=true;q.messages=false;auto summaries=dx::countrySummaries(store.rows(q));bool found=false;for(const auto& summary:summaries)if(summary.country=="Germany"){CHECK(summary.countryEstimates==1&&summary.distanceSamples==2);found=true;}CHECK(found);
  auto stats=store.stats(time(nullptr));CHECK(stats.farthestReception&&stats.farthestReception->countryDistanceGrid.empty()&&stats.farthestCountry=="Germany");
  store.countryDistances(false,home);for(const auto& row:store.rows(q))CHECK(row.decode.countryDistanceGrid.empty()&&(row.decode.position||!row.decode.km));
  store.countryDistances(true,{NAN,NAN});for(const auto& row:store.rows(q))CHECK(row.decode.countryDistanceGrid.empty());
  store.countryDistances(true,home);store.exportRows(q,folder/"export.json",false);std::ifstream exported(folder/"export.json");dx::Json data;exported>>data;CHECK(data.dump().find("distance_estimate_grid")!=std::string::npos);store.exportRows(q,folder/"export.csv",true);
  size_t index=0;for(const auto& [id,d]:store.logRows(0))CHECK(d.json()==original[index++]);CHECK(index==original.size());store.setSetting("ui",{{"history_all",true}});
 }
 {
  dx::Engine engine(folder/"profile",folder/"dx_mock_valid_small",home,country);auto wait=[&](auto condition){auto until=dx::monotonicMs()+10000;while(dx::monotonicMs()<until){if(condition(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(20));}return false;};
  auto estimated=[](const dx::Snapshot& s){for(const auto& row:s.table)if(row.decode.identity.transmitter=="DL1ABC")return !row.decode.countryDistanceGrid.empty();return false;};
  CHECK(wait([&](const auto& s){return s.historyAll&&estimated(s);}));auto record=engine.snapshot()->stats.farthest;CHECK(record>0);
  for(const auto& row:engine.snapshot()->table)if(row.decode.identity.transmitter=="DL1ABC")engine.select(row.decode);
  CHECK(wait([](const auto& s){return s.detailSelection&&!s.detailSelection->countryDistanceGrid.empty();}));
  engine.settings({{"approximate_country_distances",false}});CHECK(wait([&](const auto& s){return s.settings.value("approximate_country_distances",true)==false&&!estimated(s)&&s.detailSelection&&s.detailSelection->countryDistanceGrid.empty();}));CHECK(engine.snapshot()->stats.farthest==record);
  engine.settings({{"approximate_country_distances",true}});CHECK(wait([&](const auto& s){return estimated(s);}));
 }
 dx::Store saved(folder/"profile/history.sqlite3");size_t index=0;for(const auto& [id,d]:saved.logRows(0))CHECK(d.json()==original[index++]);CHECK(index==original.size());
 {
  dx::Store russian(folder/"russia.sqlite3");CHECK(russian.loadCountryAreas(argv[1]));russian.countryDistances(true,home);
  dx::Decode d;d.utc=1792127655;d.slotMs=d.utc*1000;d.mode="FT8";d.dial=14074000;d.snr=-19;d.message="RA0AF RA4ABH R+02";
  CHECK(russian.insert(d,home,[](const auto&){return std::make_pair("European Russia","EU");}));
  auto evidence=russian.logRows(0).front().second.json();auto rows=russian.rows({});CHECK(rows.size()==1);const auto& received=rows.front().decode;
  CHECK(received.country=="European Russia"&&received.identity.transmitter=="RA4ABH"&&received.identity.called=="RA0AF");
  CHECK(received.km&&received.countryDistanceGrid==geometry.nearestCountryGrid("Russia",home)->grid&&!received.position&&!received.bearing&&!received.distanceRecord);
  CHECK(!evidence.at("km").is_number()&&evidence.at("grid")==""&&russian.logRows(0).front().second.json()==evidence);
 }
 std::cout<<"Nearest country grids, holes/islands/dateline, cache, sorting/filters, live setting changes, defaults and unchanged reception evidence passed\n";
}
