// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <core.h>
#include <utils/cty.h>
#include <imgui_internal.h>
#include <fstream>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <utils/opengl_include_code.h>
namespace utils {void loadCTY(const char*,const char*,CTY&);}
namespace {
std::pair<std::string,std::string> countryFor(const utils::CTY& database,const std::string& input){
 std::string call=dx::upper(input);if(call.empty()||call.front()=='<'||call=="CQ"||call=="QRZ"||call=="DE")return {};
 const utils::CTY::DXCC* selected=nullptr;size_t length=0;
 for(const auto& entity:database.dxcc)for(const auto& prefix:entity.prefixes)if(prefix.exact&&call==prefix.value)return {dx::countryName(entity.name),entity.continent};
 // Portable operating prefixes override the home call. Routine /P and /M
 // suffixes do not claim an operating country.
 auto slash=call.find('/');if(slash!=std::string::npos){auto first=call.substr(0,slash),last=call.substr(slash+1);auto shortPrefix=[](const std::string& s){return s.size()<=4&&s!="P"&&s!="M"&&s!="MM"&&s!="AM"&&s!="QRP"&&s.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")!=std::string::npos;};if(shortPrefix(first))call=first;else if(shortPrefix(last))call=last;else call=first;}
 bool ambiguous=false;for(const auto& entity:database.dxcc)for(const auto& prefix:entity.prefixes)if(!prefix.exact&&call.rfind(prefix.value,0)==0){if(prefix.value.size()>length){selected=&entity;length=prefix.value.size();ambiguous=false;}else if(prefix.value.size()==length&&selected&&selected->name!=entity.name)ambiguous=true;}
 if(!selected||ambiguous)return {};return {dx::countryName(selected->name),selected->continent};
}
}
SDRPP_MOD_INFO{"advanced_ft8_ft4_dx_decoder","Advanced FT8/FT4 DX Decoder","ReLLL",DX_VERSION_MAJOR,DX_VERSION_MINOR,DX_VERSION_PATCH,1};
DXChannel::DXChannel(DXNotebook* parent,dx::Preset p,double center):owner(parent),preset(std::move(p)){
 name="dx_notebook_"+preset.mode+"_"+std::to_string(preset.dial);
 vfo=sigpath::iqFrontEnd.addVFO(name,24000,3000,preset.dial+1500-center);
 if(!vfo)return;
 usb.init(&vfo->out,dsp::demod::SSB<dsp::stereo_t>::Mode::USB,3000,24000,0,0);
 resampler.init(nullptr,24000,12000);sink.init(&usb.out,audio,this);sink.start();usb.start();
}
DXChannel::~DXChannel(){accept=false;if(vfo){sink.stop();usb.stop();sigpath::iqFrontEnd.removeVFO(name);}}
void DXChannel::audio(dsp::stereo_t* input,int count,void* context){
 auto* c=static_cast<DXChannel*>(context);++c->blocks;if(c->resetRequested.exchange(false))c->capture.reset();if(!c->accept||count<=0||count>1000000){c->capture.reset();c->progress=0;return;}
 std::vector<dsp::stereo_t> out(size_t(count)+128);int size=c->resampler.process(count,input,out.data());if(size<=0)return;
 std::vector<float> mono(size_t(size),0);double power=0;for(int i=0;i<size;i++){mono[i]=out[i].l;power+=mono[i]*mono[i];}
 c->level=10*std::log10(std::max(1e-12,power/size));c->streamMs=sigpath::iqFrontEnd.getCurrentStreamTime();auto job=c->capture.feed(mono.data(),mono.size(),c->streamMs,c->preset.dial,c->preset.mode);c->samples=c->capture.samples();c->discontinuities=c->capture.discontinuities();
 c->progress=c->capture.progress();if(job){if(c->replay)job->source="replay";c->owner->engine().enqueue(std::move(*job));}
}
DXNotebook::DXNotebook(std::string name):name_(std::move(name)){
 const char* dataRoot=std::getenv("ADVANCED_DX_DATA_ROOT");root_=dataRoot&&*dataRoot?std::filesystem::path(dataRoot):std::filesystem::path(std::getenv("HOME"))/"Library/Application Support/SDR++Brown DX Notebook";
 core::configManager.acquire();auto grid=core::configManager.conf.value("operatorLocation","KM69");core::configManager.release();home_=dx::gridPoint(grid).value_or(dx::Point{});
 Dl_info module{};dladdr(static_cast<const void*>(&_INFO_),&module);auto binary=std::filesystem::path(module.dli_fname).parent_path();
 // Brown adds regional tables after cty.dat. Those describe provinces rather
 // than DXCC countries and must not inflate the countries-heard statistic.
 core::configManager.acquire();auto resources=core::configManager.conf.value("resourcesDirectory","");core::configManager.release();utils::loadCTY((resources+"/cty/cty.dat").c_str(),"",countries_);
 tiles_=std::make_unique<dx::Tiles>(root_/"map-cache");
 engine_=std::make_unique<dx::Engine>(root_,binary/"dx_decoder",home_,[this](const std::string& call){return countryFor(countries_,call);});
 std::ifstream file(binary/"world.geojson");auto world=dx::Json::parse(file,nullptr,false);if(!world.is_discarded())for(const auto& f:world["features"]){
  auto coords=f["geometry"]["coordinates"];if(f["geometry"]["type"]=="Polygon")coords=dx::Json::array({coords});
  MapCountry country;country.name=f["properties"].value("NAME_EN",f["properties"].value("NAME",""));for(const auto& polygon:coords){std::vector<std::vector<dx::Point>> rings;for(const auto& ring:polygon){std::vector<dx::Point> line;for(const auto& v:ring)line.push_back({v[1].get<double>(),v[0].get<double>()});world_.push_back(line);rings.push_back(std::move(line));}country.polygons.push_back(std::move(rings));}double largest=-1;for(size_t index=0;index<country.polygons.size();index++){const auto& ring=country.polygons[index][0];double area=0;for(size_t i=1;i<ring.size();i++)area+=ring[i-1].lon*ring[i].lat-ring[i].lon*ring[i-1].lat;if(std::abs(area)>largest){largest=std::abs(area);country.labelPolygon=index;}}
  const auto& props=f["properties"];for(const char* key:{"NAME_EN","NAME","ADMIN","NAME_LONG","FORMAL_EN"})if(props.contains(key)&&props[key].is_string())country.keys.push_back(dx::countryOutlineKey(props[key].get<std::string>()));for(const auto& polygon:country.polygons){country.offlineMeshes.push_back(dx::countryMesh(polygon,false));country.onlineMeshes.push_back(dx::countryMesh(polygon,true));}dx::Point fallback{props.value("LABEL_Y",0.),props.value("LABEL_X",0.)};if(!country.polygons.empty())country.label=dx::countryCenter(country.polygons[country.labelPolygon][0],fallback);mapCountries_.push_back(std::move(country));
  const auto& p=f["properties"];if(p.contains("LABEL_X")&&p.contains("LABEL_Y"))labels_.push_back({p.value("NAME_EN",p.value("NAME","")),{p["LABEL_Y"].get<double>(),p["LABEL_X"].get<double>()}});
 }
 gui::menu.registerEntry(name_,[](void* p){static_cast<DXNotebook*>(p)->sidebar();},this,this);
 drawHandler_={[](ImGuiContext*,void* p){static_cast<DXNotebook*>(p)->draw();},this};gui::mainWindow.onWaterfallDrawn.bindHandler(&drawHandler_);
 inputHandler_={[](ImGui::WaterFall::InputHandlerArgs,void* p){auto* s=static_cast<DXNotebook*>(p);if(!s->enabled_||(!s->open_&&!s->settingsVisible_))return;
  auto mouse=ImGui::GetMousePos();bool inside=mouse.x>=s->windowMin_.x&&mouse.x<=s->windowMax_.x&&mouse.y>=s->windowMin_.y&&mouse.y<=s->windowMax_.y;
  inside=(s->open_&&inside)||(s->settingsVisible_&&mouse.x>=s->settingsMin_.x&&mouse.x<=s->settingsMax_.x&&mouse.y>=s->settingsMin_.y&&mouse.y<=s->settingsMax_.y);
  bool clicked=false,down=false;for(int i=0;i<3;i++){clicked|=ImGui::IsMouseClicked(i);down|=ImGui::IsMouseDown(i);}if(clicked&&(inside||s->popup_))s->ownsGesture_=true;
  auto* window=ImGui::FindWindowByName("Advanced FT8/FT4 DX Decoder###dx_notebook");auto* active=GImGui->ActiveIdWindow;bool focused=active&&window&&active->RootWindow==window&&ImGui::GetActiveID()!=0;
  auto* settings=ImGui::FindWindowByName("Advanced FT8/FT4 DX Decoder Settings###dx_notebook_settings");focused|=active&&settings&&active->RootWindow==settings&&ImGui::GetActiveID()!=0;
  if(inside||s->ownsGesture_||s->popup_||(focused&&!clicked)){gui::waterfall.inputHandled=true;gui::mainWindow.lockWaterfallControls=true;}if(!down)s->ownsGesture_=false;
 },this};gui::waterfall.onInputProcess.bindHandler(&inputHandler_);
}
DXNotebook::~DXNotebook(){gui::mainWindow.onWaterfallDrawn.unbindHandler(&drawHandler_);gui::waterfall.onInputProcess.unbindHandler(&inputHandler_);channels_.clear();gui::menu.removeEntry(name_);engine_.reset();if(homeIcon_)glDeleteTextures(1,&homeIcon_);}
void DXNotebook::enable(){enabled_=true;open_=true;present_=true;engine_->resetStats();if(settingsLoaded_&&clearOnStart_){tableCleared_=mapCleared_=int64_t(time(nullptr))+1;selected_.reset();tableQuery_.offset=0;save();refresh();}}
void DXNotebook::disable(){enabled_=false;channels_.clear();engine_->active(false);}
void DXNotebook::metadata(){
 core::configManager.acquire();auto homeGrid=core::configManager.conf.value("operatorLocation","");auto operatorCall=core::configManager.conf.value("operatorCallsign","");core::configManager.release();if(auto point=dx::gridPoint(homeGrid);point&&(point->lat!=home_.lat||point->lon!=home_.lon)){home_=*point;engine_->home(home_);}
 double center=gui::waterfall.getCenterFrequency(),rate=sigpath::iqFrontEnd.getSampleRate();bool playing=gui::mainWindow.isPlaying();
 if(center!=center_||rate!=rate_){channels_.clear();center_=center;rate_=rate;}
 auto eligible=dx::eligible(center,gui::waterfall.getBandwidth(),presets_);
 eligible.erase(std::remove_if(eligible.begin(),eligible.end(),[&](const auto& p){return p.mode=="FT8"?!ft8_:!ft4_;}),eligible.end());
 bool same=channels_.size()==eligible.size();if(same)for(size_t i=0;i<eligible.size();i++)same&=channels_[i]->preset.dial==eligible[i].dial&&channels_[i]->preset.mode==eligible[i].mode;
 if(!same){channels_.clear();for(auto p:eligible)channels_.push_back(std::make_unique<DXChannel>(this,p,center));}
 core::configManager.acquire();bool replay=core::configManager.conf.value("source","")=="File";core::configManager.release();
 auto now=ImGui::GetTime();for(auto& c:channels_){bool ready=enabled_&&!paused_&&playing;if(ready!=c->accept.load()){c->resetRequested=true;c->progress=0;}c->accept=ready&&c->vfo;c->replay=replay;
  auto blocks=c->blocks.load();if(!ready||blocks!=c->lastBlocks){c->lastBlocks=blocks;c->lastData=now;c->nextRetry=now+3;}
  else if(now-c->lastData>3&&now>c->nextRetry){auto preset=c->preset;auto retries=c->reconnects+1;c.reset();c=std::make_unique<DXChannel>(this,preset,center);c->reconnects=retries;c->lastData=now;c->nextRetry=now+std::min(30.,double(retries)*3);}
 }engine_->active(enabled_&&!paused_&&playing&&!channels_.empty());
 std::string bands;for(auto& c:channels_){if(!bands.empty())bands+=',';bands+=dx::bandName(c->preset.dial);}engine_->alertContext(bands,ft8_,ft4_);
 dx::GridTrackerContext forwarding;forwarding.grid=homeGrid;forwarding.call=operatorCall;for(auto& c:channels_)forwarding.channels.push_back(c->preset);engine_->gridTrackerContext(std::move(forwarding));
}
std::string DXNotebook::handleDebugCommand(const std::string& command,const std::string& args){std::lock_guard<std::mutex> lock(uiMutex_);
 if(command=="status"){auto s=engine_->snapshot();auto cache=tiles_->stats();dx::Json channels=dx::Json::array();for(auto& c:channels_)channels.push_back({{"mode",c->preset.mode},{"dial",c->preset.dial},{"blocks",c->blocks.load()},{"progress",c->progress.load()},{"level_dbfs",c->level.load()},{"samples",c->samples.load()},{"discontinuities",c->discontinuities.load()},{"stream_ms",c->streamMs.load()}});
  return dx::Json{{"version",DX_VERSION},{"channels",channels},{"paused",paused_},{"jobs",s->decodedJobs},{"skipped",s->skipped},{"queue",s->queued},{"pending_writes",s->pendingWrites},{"countries",s->stats.countries},{"stations",s->stats.stations},{"messages",s->stats.messages},{"last_decode_ms",s->lastDecodeMs},{"helper_pid",s->helperPid},{"error",s->error},{"query_ms",s->queryMs},{"marker_counts",mapView_.counts},{"sound",alerts_.value("sound",false)},{"sort",sort_},{"descending",descending_},{"center_hz",center_},{"night",mapView_.night},{"map_lon",mapView_.lon},{"map_lat",mapView_.lat},{"exchange_paths",s->paths.size()},{"map_zoom",mapView_.zoom},{"map_cache",{{"requests",cache.requests},{"disk_hits",cache.diskHits},{"bytes",cache.diskBytes},{"resident",cache.resident},{"wanted",cache.wanted},{"ready",cache.ready},{"viewport_ms",cache.viewportMs}}},{"map_base",mapView_.base},{"country_labels",mapView_.labels},{"map_rows",s->mapCount},{"csv_path",s->csvPath},{"csv_bytes",s->csvBytes},{"csv_error",s->csvError},{"csv_catching_up",s->csvCatchingUp},{"arrivals",s->arrivals.size()},{"newest_reception",s->newestReceptionUtc},{"direction_stations",s->direction.stations},{"direction_degrees",s->direction.degrees?dx::Json(*s->direction.degrees):dx::Json(nullptr)},{"direction_alignment",s->direction.alignment},{"farthest_country",s->stats.farthestCountry},{"table_rows",s->tableCount},{"message_kind",int(tableFilter_.messageKind)},{"highlight_countries",mapView_.highlightCountries},{"table_cleared",tableCleared_},{"map_cleared",mapCleared_}}.dump();}
 if(command=="retry"){engine_->retry();return "{}";}if(command=="show"){open_=true;present_=true;return "{}";}if(command=="settings"){settingsOpen_=true;settingsPresent_=true;return "{}";}
 if(command=="gridtracker_status"){auto s=engine_->snapshot()->gridTracker;return dx::Json{{"enabled",s.enabled},{"sent",s.sent},{"last_reply",s.lastReply},{"error",s.error},{"address",gridTrackerHost_},{"port",gridTrackerPort_}}.dump();}
 if(command=="pause"){paused_=args=="1";return "{}";}if(command=="ft8"){ft8_=args=="1";save();return "{}";}if(command=="ft4"){ft4_=args=="1";save();return "{}";}
 if(command=="map_preview"){auto j=dx::Json::parse(args,nullptr,false);if(!j.is_object())return "{}";mapView_.zoom=std::clamp(j.value("zoom",mapView_.zoom),1.,mapView_.base?65536.:32.);mapView_.lon=std::remainder(j.value("lon",mapView_.lon),360.);mapView_.lat=std::clamp(j.value("lat",mapView_.lat),-85.,85.);mapView_.base=std::clamp(j.value("base",mapView_.base),0,2);return "{}";}
 if(command=="view"){auto settings=dx::Json::parse(args,nullptr,false);if(!settings.is_object())return "{\"error\":\"Invalid view settings\"}";view_=std::clamp(settings.value("view",view_),0,2);tableTime_=std::clamp(settings.value("table_time",tableTime_),0,12);mapTime_=std::clamp(settings.value("map_time",mapTime_),0,12);messages_=settings.value("messages",messages_);save();refresh();return "{}";}
 return "{\"error\":\"Unknown command\"}";
}
std::pair<std::string,std::string> DXNotebook::lookupCountry(const std::string& call)const{return countryFor(countries_,call);}
MOD_EXPORT void _INIT_(){}
MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name){return new DXNotebook(std::move(name));}
MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance){delete instance;}
MOD_EXPORT void _END_(){}
