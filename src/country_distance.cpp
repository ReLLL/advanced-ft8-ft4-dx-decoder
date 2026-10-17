// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "location_check.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
namespace dx {
namespace {
Point nearestEdge(Point home,Point a,Point b){
 // Match the bundled polygon's straight longitude/latitude edges, while
 // minimizing spherical distance. A great-circle interpolation would bow long
 // borders away from the actual geometry used by the point-in-polygon test.
 auto point=[&](double t){return Point{a.lat+(b.lat-a.lat)*t,std::remainder(a.lon+std::remainder(b.lon-a.lon,360.)*t,360.)};};
 double low=0,high=1;for(int i=0;i<32;++i){double left=low+(high-low)/3,right=high-(high-low)/3;if(distanceBearing(home,point(left)).first<distanceBearing(home,point(right)).first)high=right;else low=left;}
 auto result=point((low+high)/2);if(distanceBearing(home,a).first<distanceBearing(home,result).first)result=a;if(distanceBearing(home,b).first<distanceBearing(home,result).first)result=b;return result;
}
std::string locator(int x,int y){
 std::string out;out+=char('A'+x/240);out+=char('A'+y/240);out+=char('0'+x/24%10);out+=char('0'+y/24%10);out+=char('A'+x%24);out+=char('A'+y%24);return out;
}
}
std::optional<LocationCheck::CountryDistance> LocationCheck::nearestCountryGrid(const std::string& country,Point home){
 if(!std::isfinite(home.lat)||!std::isfinite(home.lon)||std::abs(home.lat)>90||std::abs(home.lon)>180)return {};
 std::string name;for(unsigned char c:upper(country))if(std::isalnum(c))name+=char(c);
 // The distance-only lookup includes explicit naming/regional aliases.
 // Never reuse the map's broader sovereignty aliases for this estimate.
 auto area=distanceKeys_.find(name);if(area==distanceKeys_.end())return {};
 std::ostringstream key;key<<area->second<<':'<<std::setprecision(17)<<home.lat<<':'<<home.lon;
 if(auto found=distances_.find(key.str());found!=distances_.end())return found->second;
 struct Candidate {double km;Point point;size_t polygon;};std::vector<Candidate> candidates;
 const auto& polygons=areas_[area->second].polygons;
 for(size_t p=0;p<polygons.size();++p){const auto& polygon=polygons[p];
  const auto& bounds=areas_[area->second].bounds[p];
  // These DXCC entities exclude separately identified overseas entities.
  const auto usa=distanceKeys_.find("UNITEDSTATESOFAMERICA"),france=distanceKeys_.find("FRANCE");
  if(usa!=distanceKeys_.end()&&area->second==usa->second&&(bounds.first.lat>50||bounds.second.lat<24))continue;
  if(france!=distanceKeys_.end()&&area->second==france->second&&bounds.second.lat<40)continue;
  if(insidePolygon(home,polygon))candidates.push_back({0,home,p});
  for(const auto& ring:polygon)for(size_t i=1;i<ring.size();++i){auto point=nearestEdge(home,ring[i-1],ring[i]);candidates.push_back({distanceBearing(home,point).first,point,p});}
 }
 std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.km<b.km;});
 std::optional<CountryDistance> best;
 for(const auto& candidate:candidates){
  // Six-character cells are at most 10.4 km corner-to-corner. Inspect the
  // adjacent cells around each competitive border projection, including holes
  // and islands. Test centers against the actual polygon, not its envelope.
  if(best&&candidate.km>best->km+21)break;
  int cx=int(std::floor((candidate.point.lon+180)*12)),cy=int(std::floor((candidate.point.lat+90)*24));
  for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){int x=((cx+dx)%4320+4320)%4320,y=cy+dy;if(y<0||y>=4320)continue;Point point{(y+.5)/24.-90,(x+.5)/12.-180};double km=distanceBearing(home,point).first;if(best&&km>=best->km)continue;
   if(insidePolygon(point,polygons[candidate.polygon]))best=CountryDistance{km,locator(x,y)};
  }
 }
 if(distances_.size()>=2048)distances_.clear();distances_[key.str()]=best;return best;
}
}
