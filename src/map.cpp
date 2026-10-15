// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "map_markers.h"
#include <core.h>
#include <cmath>
#include <map>
#include <set>
#include <ctime>
#include <utils/opengl_include_code.h>
void DXNotebook::map(const std::vector<dx::Row>& rows){
 const bool online=mapView_.base!=0,homeKnown=std::isfinite(home_.lat)&&std::isfinite(home_.lon);const double maxZoom=online?65536.:32.;
 bool changed=false;if(ImGui::SmallButton("+")){mapView_.zoom=std::min(maxZoom,mapView_.zoom*1.35);changed=true;}ImGui::SameLine();if(ImGui::SmallButton("-")){mapView_.zoom=std::max(1.,mapView_.zoom/1.35);changed=true;}ImGui::SameLine();if(ImGui::SmallButton("World")){mapView_.zoom=1;mapView_.lon=mapView_.lat=0;changed=true;}ImGui::SameLine();ImGui::BeginDisabled(!homeKnown);if(ImGui::SmallButton("Home")){mapView_.lon=home_.lon;mapView_.lat=home_.lat;mapView_.zoom=4;changed=true;}ImGui::EndDisabled();ImGui::SameLine();
 if(ImGui::SmallButton("Fit")){std::vector<dx::Point> points;if(homeKnown)points.push_back(home_);for(auto& r:rows)if(r.decode.position)points.push_back(*r.decode.position);double left=180,right=-180,top=-90,bottom=90,anchor=homeKnown?home_.lon:points.empty()?0:points.front().lon;for(auto p:points){double lon=anchor+std::remainder(p.lon-anchor,360.);left=std::min(left,lon);right=std::max(right,lon);bottom=std::min(bottom,p.lat);top=std::max(top,p.lat);}if(!points.empty()){mapView_.lon=std::remainder((left+right)/2,360.);mapView_.lat=(top+bottom)/2;mapView_.zoom=std::clamp(std::min(320./std::max(10.,right-left),150./std::max(10.,top-bottom)),1.,32.);changed=true;}}
 std::set<std::string> visibleModes;bool extraModes=false;for(const auto& row:rows)if(row.decode.position){visibleModes.insert(row.decode.mode);extraModes|=dx::modeMarker(row.decode.mode)!=dx::MarkerShape::Circle;}
 if(extraModes){ImGui::SameLine();ImGui::TextDisabled("Mode shapes");if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextDisabled("Shape follows the latest reception in each marker");for(const auto& mode:dx::modeSpecs())if(visibleModes.count(std::string(mode.name))){auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({14,ImGui::GetTextLineHeight()});dx::drawModeMarker(ImGui::GetWindowDrawList(),mode.name,{p.x+7,p.y+ImGui::GetTextLineHeight()/2},5,IM_COL32(180,214,217,255));ImGui::SameLine();ImGui::TextUnformatted(mode.name.data());}ImGui::EndTooltip();}}
 if(mapView_.snrColors){ImGui::TextDisabled("SNR: -30 / -10 / +10 dB");ImGui::SameLine();auto at=ImGui::GetCursorScreenPos();ImGui::Dummy({120,10});auto draw=ImGui::GetWindowDrawList();auto color=[](double value){auto c=dx::snrMapColor(value);return ImGui::ColorConvertFloat4ToU32({c[0],c[1],c[2],c[3]});};draw->AddRectFilledMultiColor(at,{at.x+60,at.y+10},color(-30),color(-10),color(-10),color(-30));draw->AddRectFilledMultiColor({at.x+60,at.y},{at.x+120,at.y+10},color(-10),color(10),color(10),color(-10));}
 auto origin=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();size.x=std::max(size.x,10.f);size.y=std::max(size.y,10.f);ImVec2 end{origin.x+size.x,origin.y+size.y};
 ImGui::InvisibleButton("##world",size,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight);bool hovered=ImGui::IsItemHovered();auto& io=ImGui::GetIO();
 double scale=std::min(size.x/360.,size.y/(online?360.:180.))*mapView_.zoom;
 auto unproject=[&](ImVec2 p){double x=(p.x-origin.x-size.x/2)/scale,y=(p.y-origin.y-size.y/2)/scale;if(!online)return dx::Point{mapView_.lat-y,std::remainder(mapView_.lon+x,360.)};auto center=dx::mercator({mapView_.lat,mapView_.lon});auto point=dx::geographic({center.lat+y/360,center.lon+x/360});point.lon=std::remainder(point.lon,360.);return point;};
 if(hovered&&io.MouseWheel!=0){auto anchor=unproject(io.MousePos);double before=mapView_.zoom;mapView_.zoom=std::clamp(before*std::pow(1.18,io.MouseWheel),1.,maxZoom);scale*=mapView_.zoom/before;auto after=unproject(io.MousePos);mapView_.lon+=std::remainder(anchor.lon-after.lon,360.);if(online){auto center=dx::mercator({mapView_.lat,mapView_.lon});center.lat+=dx::mercator(anchor).lat-dx::mercator(after).lat;mapView_.lat=dx::geographic(center).lat;}else mapView_.lat+=anchor.lat-after.lat;changed=true;}
 if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){auto center=unproject({float(origin.x+size.x/2-io.MouseDelta.x),float(origin.y+size.y/2-io.MouseDelta.y)});mapView_.lon=center.lon;mapView_.lat=center.lat;changed=true;}
 mapView_.lon=std::remainder(mapView_.lon,360.);mapView_.lat=std::clamp(mapView_.lat,online?-85.05112878:-90.,online?85.05112878:90.);
 auto project=[&](dx::Point p,int copy=0){double lon=std::remainder(p.lon-mapView_.lon,360.)+copy*360.;double y=online?(dx::mercator(p).lat-dx::mercator({mapView_.lat,mapView_.lon}).lat)*360:mapView_.lat-p.lat;return ImVec2{float(origin.x+size.x/2+lon*scale),float(origin.y+size.y/2+y*scale)};};
 auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(origin,end,true);draw->AddRectFilled(origin,end,IM_COL32(17,30,37,255));
 if(online){double world=360*scale;int z=std::clamp(int(std::ceil(std::log2(world/256))),0,19),n=1<<z;double tile=world/n;while(z>0&&(std::ceil(size.x/tile)+2)*(std::ceil(size.y/tile)+2)>128){--z;n=1<<z;tile=world/n;}auto center=dx::mercator({mapView_.lat,mapView_.lon});double left=origin.x+size.x/2-center.lon*world,top=origin.y+size.y/2-center.lat*world;
  int x0=int(std::floor((origin.x-left)/tile)),x1=int(std::floor((end.x-left)/tile)),y0=std::max(0,int(std::floor((origin.y-top)/tile))),y1=std::min(n-1,int(std::floor((end.y-top)/tile)));
  std::vector<dx::TileKey> keys;for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)keys.push_back({z,(x%n+n)%n,y,mapView_.base});tiles_->visible(keys);
  for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){int wrapped=(x%n+n)%n;for(int depth=1;depth<=z;depth++){int subdivisions=1<<depth;auto texture=tiles_->texture({z-depth,wrapped/subdivisions,y/subdivisions,mapView_.base});if(!texture)continue;ImVec2 uv{float(wrapped%subdivisions)/subdivisions,float(y%subdivisions)/subdivisions};draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{float(left+x*tile),float(top+y*tile)},{float(left+(x+1)*tile),float(top+(y+1)*tile)},uv,{uv.x+1.f/subdivisions,uv.y+1.f/subdivisions});break;}}
  for(auto key:tiles_->ready())if(key.layer==mapView_.base&&key.z>z&&key.z<=z+2){int count=1<<key.z;double pixels=world/count;for(int copy=-1;copy<=1;copy++){ImVec2 a{float(left+(key.x+copy*count)*pixels),float(top+key.y*pixels)},b{float(a.x+pixels),float(a.y+pixels)};if(a.x>end.x||b.x<origin.x||a.y>end.y||b.y<origin.y)continue;if(auto texture=tiles_->texture(key))draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(texture)),a,b);}}
  for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)if(auto texture=tiles_->texture({z,(x%n+n)%n,y,mapView_.base}))draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{float(left+x*tile),float(top+y*tile)},{float(left+(x+1)*tile),float(top+(y+1)*tile)});
 }else tiles_->visible({});
 auto snapshot=engine_->snapshot();
 if(mapView_.highlightCountries){
  if(highlightedCountries_!=snapshot->mapHeardCountries||heardCountryOutlines_.size()!=mapCountries_.size()){
   highlightedCountries_=snapshot->mapHeardCountries;std::set<std::string> keys;for(const auto& name:highlightedCountries_)keys.insert(dx::countryOutlineKey(name));heardCountryOutlines_.assign(mapCountries_.size(),false);
   for(size_t i=0;i<mapCountries_.size();i++)for(const auto& key:mapCountries_[i].keys)if(keys.count(key)){heardCountryOutlines_[i]=true;break;}
  }
  ImU32 color=ImGui::ColorConvertFloat4ToU32({mapView_.countryColor[0],mapView_.countryColor[1],mapView_.countryColor[2],mapView_.countryColor[3]});auto flags=draw->Flags;draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
  double centerY=online?dx::mercator({mapView_.lat,mapView_.lon}).lat*360:mapView_.lat;
  for(size_t i=0;i<mapCountries_.size();i++)if(heardCountryOutlines_[i])for(const auto& mesh:online?mapCountries_[i].onlineMeshes:mapCountries_[i].offlineMeshes)for(int copy=-1;copy<=1;copy++){
   auto screen=[&](dx::Point p){return ImVec2{float(origin.x+size.x/2+(p.lon-mapView_.lon+copy*360)*scale),float(origin.y+size.y/2+(online?p.lat-centerY:centerY-p.lat)*scale)};};
   auto a=screen(mesh.min),b=screen(mesh.max);if(std::min(a.x,b.x)>end.x||std::max(a.x,b.x)<origin.x||std::min(a.y,b.y)>end.y||std::max(a.y,b.y)<origin.y)continue;
   std::vector<ImVec2> points;points.reserve(mesh.points.size());for(auto p:mesh.points)points.push_back(screen(p));
   for(size_t j=0;j+2<mesh.indices.size();j+=3)draw->AddTriangleFilled(points[mesh.indices[j]],points[mesh.indices[j+1]],points[mesh.indices[j+2]],color);
  }
  draw->Flags=flags;
 }
 std::string hoveredCountry,hoveredPath;
 if(hovered){dx::Point pointer=unproject(io.MousePos);
  auto contains=[&](const std::vector<dx::Point>& ring){bool inside=false;for(size_t i=0,j=ring.size()-1;i<ring.size();j=i++){double x=pointer.lon+std::remainder(ring[i].lon-pointer.lon,360.),z=pointer.lon+std::remainder(ring[j].lon-pointer.lon,360.);if(std::abs(x-z)>180)continue;if((ring[i].lat>pointer.lat)!=(ring[j].lat>pointer.lat)&&pointer.lon<(z-x)*(pointer.lat-ring[i].lat)/(ring[j].lat-ring[i].lat)+x)inside=!inside;}return inside;};
  for(const auto& country:mapCountries_){bool inside=false;for(const auto& polygon:country.polygons){if(polygon.empty()||polygon[0].empty()||!contains(polygon[0]))continue;bool hole=false;for(size_t i=1;i<polygon.size();i++)if(!polygon[i].empty()&&contains(polygon[i]))hole=true;if(!hole)inside=true;}if(inside){hoveredCountry=country.name;break;}}
 }
 auto line=[&](const std::vector<dx::Point>& points,ImU32 color,float thickness){
  for(int copy=-1;copy<=1;copy++)for(size_t i=1;i<points.size();i++){auto a=project(points[i-1],copy),b=project(points[i],copy);if(std::abs(a.x-b.x)<180*scale)draw->AddLine(a,b,color,thickness);}
 };
 if(!online)for(const auto& coast:world_)line(coast,IM_COL32(92,119,121,220),1);
 if(mapView_.grids){for(int lon=-180;lon<180;lon+=20)line({{-90,double(lon)},{90,double(lon)}},IM_COL32(74,95,106,100),1);for(int lat=-80;lat<90;lat+=10)line({{double(lat),-180},{double(lat),-60},{double(lat),60},{double(lat),179.99}},IM_COL32(74,95,106,100),1);
  if(mapView_.zoom>2)for(int a=0;a<18;a++)for(int b=0;b<18;b++){char label[3]={char('A'+a),char('A'+b),0};auto p=project({-85.+10*b,-170.+20*a});draw->AddText(p,IM_COL32(105,131,138,160),label);}
 }
 const float fontSize=ImGui::GetFontSize()*.85f;
 if(mapView_.base!=2&&mapView_.labels){if(labelScale_!=scale||labelFontSize_!=fontSize||labelBase_!=mapView_.base){labelScale_=scale;labelFontSize_=fontSize;labelBase_=mapView_.base;fittedLabels_.assign(mapCountries_.size(),false);for(size_t i=0;i<mapCountries_.size();i++){const auto& c=mapCountries_[i];auto text=ImGui::GetFont()->CalcTextSizeA(fontSize,FLT_MAX,0,c.name.c_str());if(!c.polygons.empty())fittedLabels_[i]=dx::labelFits(c.label,(text.x+6)/scale,(text.y+4)/scale*(online?std::cos(c.label.lat*3.141592653589793/180):1.),c.polygons[c.labelPolygon]);}}
  for(size_t i=0;i<mapCountries_.size();i++)if(fittedLabels_[i]){const auto& c=mapCountries_[i];auto p=project(c.label),text=ImGui::GetFont()->CalcTextSizeA(fontSize,FLT_MAX,0,c.name.c_str());ImVec2 at{p.x-text.x/2,p.y-text.y/2};if(at.x>=origin.x&&at.y>=origin.y&&at.x+text.x<=end.x&&at.y+text.y<=end.y)draw->AddText(ImGui::GetFont(),fontSize,at,IM_COL32(148,163,163,140),c.name.c_str());}
 }
 static int64_t minute=-1;static dx::Point sun;auto now=time(nullptr);if(now/60!=minute){minute=now/60;sun=dx::sunPosition(now);}
 if(mapView_.night){constexpr double rad=3.141592653589793/180.;
  auto fillFlags=draw->Flags;draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
  auto halfDay=[&](double lat){double v=-std::tan(lat*rad)*std::tan(sun.lat*rad);return std::acos(std::clamp(v,-1.,1.))/rad;};
  auto rawProject=[&](double lat,double lon){double y=online?(dx::mercator({lat,lon}).lat-dx::mercator({mapView_.lat,mapView_.lon}).lat)*360:mapView_.lat-lat;return ImVec2{float(origin.x+size.x/2+(lon-mapView_.lon)*scale),float(origin.y+size.y/2+y*scale)};};
  for(int lat=-90;lat<90;lat++){double top=lat,bottom=lat+1,h0=halfDay(top),h1=halfDay(bottom);
   for(int copy=-2;copy<=2;copy++){double start=sun.lon+360*copy;ImVec2 polygon[]={rawProject(top,start+h0),rawProject(top,start+360-h0),rawProject(bottom,start+360-h1),rawProject(bottom,start+h1)};draw->AddConvexPolyFilled(polygon,4,IM_COL32(3,8,17,100));draw->AddLine(polygon[0],polygon[3],IM_COL32(216,182,106,90),1);draw->AddLine(polygon[1],polygon[2],IM_COL32(216,182,106,90),1);}
  }
  draw->Flags=fillFlags;
 }
 const double spectrumCenter=gui::waterfall.getCenterFrequency()+gui::waterfall.getViewOffset(),spectrumWidth=gui::waterfall.getViewBandwidth();
 const std::pair<double,double> spectrum{spectrumCenter-spectrumWidth/2,spectrumCenter+spectrumWidth/2};
 for(size_t index=0;index<snapshot->paths.size();index++){
  const auto& path=snapshot->paths[index];if(path.points.size()<2||!dx::frequencyVisible(path.frequency,spectrum))continue;
  std::vector<ImVec2> arc;double longitude=path.points.front().lon,previous=longitude;auto first=project(path.points.front());
  for(auto point:path.points){longitude+=std::remainder(point.lon-previous,360.);previous=point.lon;auto p=project(point);p.x=float(first.x+(longitude-path.points.front().lon)*scale);arc.push_back(p);}
  double dx=arc.back().x-arc.front().x,dy=arc.back().y-arc.front().y,length=std::hypot(dx,dy);if(length<1)continue;
  double nx=-dy/length,ny=dx/length;if(ny>0){nx=-nx;ny=-ny;}double lift=std::clamp(length*.2,12.,80.);
  for(size_t i=0;i<arc.size();i++){double bow=std::sin(3.141592653589793*i/(arc.size()-1))*lift;arc[i].x+=float(nx*bow);arc[i].y+=float(ny*bow);}
  auto color=IM_COL32(20,55,125,235-int(index)*10);double phase=mapView_.animatePaths?std::fmod(ImGui::GetTime()*24.,9.):0.;double travelled=0;
  for(size_t i=1;i<arc.size();i++){
   ImVec2 a=arc[i-1],b=arc[i];double segment=std::hypot(b.x-a.x,b.y-a.y);if(segment<.001)continue;
   for(int copy=-1;copy<=1;copy++){
    double offset=copy*360*scale,t0=0,t1=1;
    auto clip=[&](double start,double delta,double low,double high){if(std::abs(delta)<1e-9)return start>=low&&start<=high;double from=(low-start)/delta,to=(high-start)/delta;if(from>to)std::swap(from,to);t0=std::max(t0,from);t1=std::min(t1,to);return t0<=t1;};
    if(!clip(a.x+offset,b.x-a.x,origin.x-3,end.x+3)||!clip(a.y,b.y-a.y,origin.y-3,end.y+3))continue;
    double begin=travelled+segment*t0,finish=travelled+segment*t1;
    for(double at=std::ceil((begin-phase)/9.)*9.+phase;at<=finish;at+=9.){double t=(at-travelled)/segment;ImVec2 p{float(a.x+offset+(b.x-a.x)*t),float(a.y+(b.y-a.y)*t)};draw->AddCircleFilled(p,1.6f,color,6);}
    if(hovered){double t=std::clamp(((io.MousePos.x-a.x-offset)*(b.x-a.x)+(io.MousePos.y-a.y)*(b.y-a.y))/(segment*segment),t0,t1);if(std::hypot(io.MousePos.x-a.x-offset-(b.x-a.x)*t,io.MousePos.y-a.y-(b.y-a.y)*t)<5)hoveredPath="Observed reply: "+path.transmitter+" -> "+path.called+"\nPositions from received grids; may be cached";}
   }
   travelled+=segment;
  }
 }
 if(!homeIcon_){auto pixels=dx::homeIconPixels();if(pixels.size()==64*64*4){GLint texture=0,rowLength=0,alignment=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glGenTextures(1,&homeIcon_);glBindTexture(GL_TEXTURE_2D,homeIcon_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glBindTexture(GL_TEXTURE_2D,GLuint(texture));glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);}}
 struct Cluster {ImVec2 p;int count=0;const dx::Decode* d=nullptr;};std::map<std::pair<int,int>,Cluster> clusters;
 for(auto& row:rows){auto& d=row.decode;if(!d.position)continue;auto p=project(*d.position);if(p.x<origin.x||p.x>end.x||p.y<origin.y||p.y>end.y)continue;auto key=std::make_pair(int((p.x-origin.x)/16),int((p.y-origin.y)/16));auto& c=clusters[key];c.count++;if(!c.d||d.utc>c.d->utc){c.p=p;c.d=&d;}}
 const Cluster* hoveredStation=nullptr;double hoverDistance=10;
 for(auto& [_,c]:clusters){bool selected=selected_&&selected_->identity.transmitter==c.d->identity.transmitter,newest=snapshot->newestReceptionUtc>0&&c.d->utc==snapshot->newestReceptionUtc;if(newest){draw->AddCircleFilled(c.p,10,IM_COL32(178,235,158,70));draw->AddCircle(c.p,8,IM_COL32(185,239,167,245),16,1.5f);}dx::drawModeMarker(draw,c.d->mode,c.p,selected?7.5f:5.5f,IM_COL32(18,41,44,220));auto snr=dx::snrMapColor(c.d->snr);auto fill=mapView_.snrColors?ImGui::ColorConvertFloat4ToU32({snr[0],snr[1],snr[2],snr[3]}):selected?IM_COL32(242,196,114,255):newest?IM_COL32(178,235,158,255):IM_COL32(98,207,178,255);dx::drawModeMarker(draw,c.d->mode,c.p,selected?6:dx::modeMarker(c.d->mode)==dx::MarkerShape::Circle?4:5,fill);if(mapView_.snrColors&&selected)dx::drawModeMarker(draw,c.d->mode,c.p,8,IM_COL32(242,196,114,255),false);if(mapView_.counts&&c.count>1){auto number=std::to_string(c.count);draw->AddText({c.p.x+5,c.p.y-8},IM_COL32(196,221,216,255),number.c_str());}
  double distance=std::hypot(io.MousePos.x-c.p.x,io.MousePos.y-c.p.y);if(hovered&&distance<hoverDistance){hoveredStation=&c;hoverDistance=distance;}
 }
 auto home=homeKnown?project(home_):ImVec2{};bool homeHovered=homeKnown&&hovered&&std::hypot(io.MousePos.x-home.x,io.MousePos.y-home.y)<12;
 if(hoveredStation&&!homeHovered&&ImGui::IsMouseReleased(1)){mapContext_=*hoveredStation->d;ImGui::OpenPopup("map_station_actions");}
 if(ImGui::BeginPopup("map_station_actions")){if(mapContext_)stationMenu(*mapContext_,rows);ImGui::EndPopup();}
 if(hoveredStation&&!homeHovered&&!ImGui::IsPopupOpen("map_station_actions")){const auto& d=*hoveredStation->d;engine_->hover(d);ImGui::BeginTooltip();ImGui::PushTextWrapPos(420);
  ImGui::Text("%s | %s",d.identity.transmitter.c_str(),d.country.empty()?"Unknown":d.country.c_str());const auto referenceUtc=int64_t(time(nullptr));ImGui::Text("%s: %s | %s | %s | %+d dB SNR",dx::ageLabel(d.utc,referenceUtc),dx::ageText(d.utc,referenceUtc).c_str(),dx::bandName(d.dial).c_str(),d.mode.c_str(),d.snr);
  if(d.km)ImGui::Text("Distance: %s%s",dx::distanceSummary(*d.km,distanceMiles_).c_str(),d.gridSource=="cached"?" | cached grid":"");else ImGui::TextUnformatted("Distance unavailable: no recent received grid");ImGui::Text("Grid: %s | %.4f, %.4f",d.identity.grid.c_str(),d.position->lat,d.position->lon);ImGui::PopTextWrapPos();ImGui::Separator();ImGui::TextDisabled("Latest 5 unique messages");
  bool ready=snapshot->hoverStation&&snapshot->hoverStation->identity.transmitter==d.identity.transmitter&&snapshot->hoverStation->dial==d.dial&&snapshot->hoverStation->mode==d.mode&&!snapshot->hoverMessages.empty();
  auto messageRow=[](const dx::Decode& message){auto color=dx::snrMapColor(message.snr);ImGui::TextColored({.60f,.76f,.86f,1},"%s",dx::localDateTime(message.utc).c_str());ImGui::SameLine(0,8);ImGui::TextColored({.92f,.94f,.95f,1},"\"%s\"",message.message.c_str());ImGui::SameLine(0,8);ImGui::TextColored({color[0],color[1],color[2],color[3]},"%+d dB SNR",message.snr);};
  if(ready)for(const auto& message:snapshot->hoverMessages)messageRow(message);else messageRow(d);
  if(hoveredStation->count>1)ImGui::TextDisabled("Latest reception in this marker (%d entries)",hoveredStation->count);ImGui::EndTooltip();if(ImGui::IsMouseClicked(0)){selected_=d;engine_->select(d);save();}
 }
 if(homeKnown){if(homeIcon_)draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(homeIcon_)),{home.x-11,home.y-11},{home.x+11,home.y+11});else draw->AddCircle(home,5,IM_COL32(242,207,128,255),12,2);}
 if(homeHovered){
#if DX_HOST_BROWN
 core::configManager.acquire();auto grid=core::configManager.conf.value("operatorLocation","");core::configManager.release();
#else
 auto grid=receiverGrid_;
#endif
 ImGui::SetTooltip("Home: %s | %.4f, %.4f\nOrigin for great-circle distances and bearings",grid.c_str(),home_.lat,home_.lon);}else if(!hoveredStation){const auto& tooltip=hoveredPath.empty()?hoveredCountry:hoveredPath;if(!tooltip.empty())ImGui::SetTooltip("%s",tooltip.c_str());}
 const char* credit=mapView_.base==2?"Sources: Esri, HERE, Garmin, USGS, Intermap, INCREMENT P, NRCan, Esri Japan, METI, Esri China (Hong Kong), Esri Korea, Esri (Thailand), NGCC, OpenStreetMap contributors, and the GIS User Community":online?"OpenStreetMap contributors | HOT style | OSM France":"Natural Earth | approximate grid centers";auto creditSize=ImGui::GetFont()->CalcTextSizeA(fontSize,FLT_MAX,size.x-14,credit);ImVec2 at{origin.x+7,end.y-creditSize.y-5};draw->AddRectFilled({at.x-3,at.y-2},{at.x+creditSize.x+3,end.y-3},IM_COL32(17,30,37,240));draw->AddText(ImGui::GetFont(),fontSize,at,IM_COL32(183,199,200,255),credit,nullptr,size.x-14);
 if(hovered&&io.MousePos.y>=at.y-2&&io.MousePos.x<=at.x+creditSize.x+3&&ImGui::IsMouseClicked(0))ImGui::OpenPopup("Map credits");
 if(ImGui::BeginPopup("Map credits")){popup_=true;if(mapView_.base==2&&ImGui::MenuItem("Esri map sources"))dx::openMapCredit(3);if(ImGui::MenuItem("OpenStreetMap data and license"))dx::openMapCredit(0);if(mapView_.base==1){if(ImGui::MenuItem("Humanitarian map style"))dx::openMapCredit(1);if(ImGui::MenuItem("OpenStreetMap France"))dx::openMapCredit(2);}ImGui::EndPopup();}
 auto error=online?tiles_->status():std::string{};if(!error.empty()){draw->AddRectFilled({origin.x+4,origin.y+4},{end.x-4,origin.y+40},IM_COL32(17,30,37,230));draw->AddText(ImGui::GetFont(),fontSize,{origin.x+7,origin.y+7},IM_COL32(238,197,130,255),error.c_str(),nullptr,std::max(30.f,size.x-14));}
 draw->PopClipRect();if(changed){save();refresh();}
}
