// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <cstdio>
#include <cfloat>
#include <core.h>

namespace {
bool text(const char* label,std::string& value){char buffer[256];std::snprintf(buffer,sizeof(buffer),"%s",value.c_str());bool changed=ImGui::InputText(label,buffer,sizeof(buffer));if(changed)value=buffer;return changed;}
}

void DXNotebook::settingsWindow() {
 settingsVisible_=false;
 if(!settingsOpen_)return;
 auto* viewport=ImGui::GetMainViewport();
 if(settingsPresent_){ImGui::SetNextWindowPos({viewport->WorkPos.x+viewport->WorkSize.x/2,viewport->WorkPos.y+viewport->WorkSize.y/2},ImGuiCond_Always,{.5f,.5f});ImGui::SetNextWindowSize({720,570},ImGuiCond_Always);ImGui::SetNextWindowFocus();settingsPresent_=false;}
 ImGui::SetNextWindowSizeConstraints({580,380},{FLT_MAX,FLT_MAX});
 ImGui::PushStyleColor(ImGuiCol_WindowBg,{.075f,.095f,.105f,1});
 ImGui::PushStyleColor(ImGuiCol_ChildBg,{.075f,.095f,.105f,1});
 bool visible=ImGui::Begin("Advanced FT8/FT4 DX Decoder Settings###dx_notebook_settings",&settingsOpen_,ImGuiWindowFlags_NoCollapse);
 settingsVisible_=true;settingsMin_=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();settingsMax_={settingsMin_.x+size.x,settingsMin_.y+size.y};
 if(visible){
  const char* pages[]={"General","Reception","Map","Alerts","Filters","History","Integrations"};
  ImGui::BeginChild("settings_navigation",{125,0},true);
  for(int i=0;i<7;i++)if(ImGui::Selectable(pages[i],settingsPage_==i))settingsPage_=i;
  ImGui::EndChild();ImGui::SameLine();
  ImGui::BeginChild("settings_content",{0,0},false);
  ImGui::TextUnformatted(pages[settingsPage_]);ImGui::Separator();bool changed=false;
  ImGui::PushItemWidth(std::min(260.f,ImGui::GetContentRegionAvail().x*.56f));
  if(settingsPage_==0){
#if !DX_HOST_BROWN
   ImGui::TextUnformatted("Home location");if(text("Maidenhead grid",receiverGrid_)){receiverGrid_=dx::upper(receiverGrid_);receiverGridError_=!receiverGrid_.empty()&&!dx::gridPoint(receiverGrid_);if(!receiverGridError_){core::configManager.acquire();core::configManager.conf["moduleInstances"][name_]["receiverGrid"]=receiverGrid_;core::configManager.release(true);if(receiverGrid_.empty()){home_={NAN,NAN};engine_->home(home_);}changed=true;}}
   if(receiverGridError_)ImGui::TextColored({1,.55f,.4f,1},"Use a four-, six- or eight-character locator.");else ImGui::TextDisabled("Your home grid sets the map origin, distances and bearings.");
   if(text("Receiver callsign (optional)",receiverCall_)){core::configManager.acquire();core::configManager.conf["moduleInstances"][name_]["receiverCall"]=receiverCall_;core::configManager.release(true);changed=true;}ImGui::Separator();
#endif
   changed|=ImGui::Checkbox("Corner notifications on waterfall",&cornerEnabled_);if(ImGui::IsItemHovered())ImGui::SetTooltip("Every new reception appears briefly, including repeats. The farthest reception stays below the list with its flag. Only the Farthest label is gold. New rows glow for two seconds.");
   ImGui::BeginDisabled(!cornerEnabled_);changed|=ImGui::SliderInt("Notification duration (seconds)",&cornerSeconds_,2,120);ImGui::EndDisabled();
   int distanceUnit=distanceMiles_?1:0;const char* units[]={"Kilometers (km)","Miles (mi)"};if(ImGui::Combo("Distance units",&distanceUnit,units,2)){distanceMiles_=distanceUnit==1;changed=true;}
   const char* views[]={"Map + table","Table only","Map only"};changed|=ImGui::Combo("View",&view_,views,3);
   changed|=ImGui::Checkbox("Link map and table filters",&link_);
   ImGui::TextWrapped("%s","By default, each launch starts with clear views and fresh session times. Saved history can be enabled in History.");
   ImGui::Separator();changed|=timeWindow("Table time window",tableTime_);changed|=timeWindow("Map time window",mapTime_);
   ImGui::BeginDisabled(historyAll_);ImGui::Spacing();ImGui::TextUnformatted("Custom UTC range");
   changed|=ImGui::InputText("From UTC",customSince_,sizeof(customSince_));changed|=ImGui::InputText("To UTC",customUntil_,sizeof(customUntil_));ImGui::TextDisabled("YYYY-MM-DD HH:MM:SS");
   if(ImGui::Button("Use range for table and map")){tableTime_=mapTime_=12;changed=true;}ImGui::EndDisabled();
  }else if(settingsPage_==1){
   ImGui::TextUnformatted("Enabled modes");changed|=ImGui::Checkbox("FT8",&ft8_);ImGui::SameLine();changed|=ImGui::Checkbox("FT4",&ft4_);
   ImGui::Spacing();ImGui::TextUnformatted("Custom frequencies");ImGui::InputText("Dial frequency in Hz",customDial_,sizeof(customDial_));
   auto add=[&](const char* mode){long long dial=0;char tail=0;if(std::sscanf(customDial_,"%lld %c",&dial,&tail)!=1||dial<=100000||dial>=10000000000LL){settingsError_="Enter a valid dial frequency.";return;}
    for(const auto& p:presets_)if(p.dial==dial&&p.mode==mode){settingsError_="This frequency and mode are already enabled.";return;}
    presets_.push_back({dial,mode});settingsError_.clear();changed=true;
   };
   if(ImGui::Button("Add FT8"))add("FT8");ImGui::SameLine();if(ImGui::Button("Add FT4"))add("FT4");
   if(!settingsError_.empty())ImGui::TextWrapped("%s",settingsError_.c_str());
   ImGui::Separator();changed|=text("Watch calls and favorites",watchCalls_);ImGui::TextDisabled("Comma-separated calls or wildcards");
   ImGui::Separator();ImGui::TextUnformatted("DX activity bearing");changed|=ImGui::SliderInt("Farthest stations",&directionStations_,1,25);ImGui::TextDisabled("Reception window: last 15 minutes");ImGui::TextWrapped("Approximate geographic bearing of distant stations heard on the displayed bands. This describes reception activity; actual propagation paths and arrival angles are not measured.");
  }else if(settingsPage_==2){
   const char* sources[]={"Offline world map","Humanitarian OpenStreetMap","Street by Esri (English)"};if(ImGui::Combo("Map source",&mapView_.base,sources,3)){mapView_.zoom=std::min(mapView_.zoom,mapView_.base?65536.:32.);changed=true;}ImGui::TextWrapped("Online maps need internet; previously viewed tiles are cached.");auto cache=tiles_->stats();ImGui::TextDisabled("Map cache: %.1f MB of 256 MB",cache.diskBytes/(1024.*1024.));ImGui::Separator();
   ImGui::TextUnformatted("Map overlays");changed|=ImGui::Checkbox("Night shading and grayline",&mapView_.night);changed|=ImGui::Checkbox("Maidenhead grid",&mapView_.grids);changed|=ImGui::Checkbox("Country names (offline and OpenStreetMap)",&mapView_.labels);changed|=ImGui::Checkbox("Station counts on markers",&mapView_.counts);
   ImGui::Separator();changed|=ImGui::Checkbox("Highlight heard countries",&mapView_.highlightCountries);ImGui::BeginDisabled(!mapView_.highlightCountries);changed|=ImGui::ColorEdit4("Country highlight",mapView_.countryColor,ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_AlphaPreviewHalf|ImGuiColorEditFlags_Float);ImGui::EndDisabled();ImGui::TextWrapped("Heard countries in the current map view. Several DXCC entities may share one country outline.");
   ImGui::Separator();ImGui::TextUnformatted("Recent exchanges");changed|=ImGui::SliderInt("Latest exchanges",&mapView_.recentPaths,0,10);changed|=ImGui::Checkbox("Animate paths",&mapView_.animatePaths);ImGui::TextWrapped("Each arc connects the transmitting station to the station it addresses. One arc per pair; general calls are excluded. Both stations need received grids. Map filters apply.");
  }else if(settingsPage_==3){
   ImGui::TextUnformatted("Notify me about");auto flag=[&](const char* label,const char* key,bool fallback){bool value=alerts_.value(key,fallback);if(ImGui::Checkbox(label,&value)){alerts_[key]=value;changed=true;}};
   flag("Distance records","distance",true);flag("New stations","station",false);flag("New countries","country",false);flag("New grids","grid",false);flag("Watch-list matches","watch",false);
   ImGui::Separator();flag("Play sound alerts","sound",false);ImGui::TextWrapped("Sound alerts start off. Preview plays only when clicked.");
   const char* soundKinds[]={"distance","station","country","grid","watch"};const char* soundLabels[]={"Distance record","New station","New country","New grid","Watch-list match"};
   auto sounds=alerts_.value("sounds",dx::Json::object());
   if(ImGui::BeginTable("alert_sounds",3,ImGuiTableFlags_SizingFixedFit)){ImGui::TableSetupColumn("Alert",ImGuiTableColumnFlags_WidthFixed,145);ImGui::TableSetupColumn("Sound",ImGuiTableColumnFlags_WidthStretch);ImGui::TableSetupColumn("Preview",ImGuiTableColumnFlags_WidthFixed,65);
    for(int i=0;i<5;i++){ImGui::PushID(soundKinds[i]);auto tone=dx::alertTone(sounds,soundKinds[i]);int choice=0;for(int j=0;j<15;j++)if(tone==dx::alertSoundNames[j])choice=j;
     ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();ImGui::TextUnformatted(soundLabels[i]);ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##sound",&choice,dx::alertSoundNames,15)){sounds[soundKinds[i]]=dx::alertSoundNames[choice];alerts_["sounds"]=sounds;changed=true;}ImGui::TableNextColumn();ImGui::BeginDisabled(choice==0);if(ImGui::Button("Preview")){if(!dx::playAlertSound(dx::alertSoundNames[choice]))settingsError_="This sound could not be played.";else settingsError_.clear();}ImGui::EndDisabled();ImGui::PopID();
    }ImGui::EndTable();
   }
   if(!settingsError_.empty())ImGui::TextWrapped("%s",settingsError_.c_str());
   ImGui::Spacing();ImGui::TextUnformatted("Alert coverage");flag("All bands","all_bands",false);
   int scope=alerts_.value("seconds",int64_t(3600))==0?1:0;const char* scopes[]={"Last hour","All history"};if(ImGui::Combo("Alert time window",&scope,scopes,2)){alerts_["seconds"]=scope?0:3600;changed=true;}
   if(ImGui::CollapsingHeader("Alert filters"))changed|=filterEditor(alertFilter_,"alerts");
  }else if(settingsPage_==4){
   if(ImGui::CollapsingHeader("Table filters",ImGuiTreeNodeFlags_DefaultOpen))changed|=filterEditor(tableFilter_,"table");
   if(ImGui::CollapsingHeader("Map filters"))changed|=filterEditor(mapFilter_,"map");
  }else if(settingsPage_==5){
   changed|=historyOptions();ImGui::TextWrapped("Saved history loads in the background. Filters still apply; uptime remains this session. Both options start off.");ImGui::Separator();ImGui::BeginDisabled(historyAll_);if(tableCleared_&&ImGui::Button("Restore table history")){tableCleared_=0;changed=true;}if(mapCleared_&&ImGui::Button("Restore map history")){mapCleared_=0;changed=true;}ImGui::EndDisabled();
   ImGui::TextUnformatted("History and export");ImGui::TextWrapped("Exports use the current table filters and time window.");
   if(ImGui::Button("Export table CSV"))engine_->exportData(tableQuery_,root_/"Advanced FT8-FT4 DX Decoder.csv",true);
   if(ImGui::Button("Export table JSON"))engine_->exportData(tableQuery_,root_/"Advanced FT8-FT4 DX Decoder.json",false);
   auto moderation=engine_->snapshot();ImGui::Separator();ImGui::Text("Ignored identities: %zu | messages: %zu",moderation->ignoredCalls.size(),moderation->ignoredMessages.size());if(!moderation->ignoredCalls.empty()||!moderation->ignoredMessages.empty()){std::string calls;for(const auto& call:moderation->ignoredCalls){if(!calls.empty())calls+=", ";calls+=call;}if(!calls.empty())ImGui::TextWrapped("%s",calls.c_str());for(const auto& message:moderation->ignoredMessages)ImGui::TextWrapped("\"%s\"",message.c_str());if(ImGui::Button("Unignore all")){engine_->unignoreAll();tableFilter_.excludeCall.clear();mapFilter_.excludeCall.clear();changed=true;}ImGui::TextWrapped("Unignore all allows future receptions; deleted records stay deleted.");}
   ImGui::Separator();ImGui::TextUnformatted("Persistent reception CSV log");ImGui::TextWrapped("Clear and startup can reset views and session stats. Ignore transmitter, after confirmation, deletes that transmitter's saved receptions. Commas, quotes and line breaks are preserved.");
   ImGui::SetNextItemWidth(std::max(150.f,ImGui::GetContentRegionAvail().x-70));if(ImGui::InputText("CSV file",csvPath_,sizeof(csvPath_),ImGuiInputTextFlags_EnterReturnsTrue)&&csvPath_[0]){csvLogPath_=csvPath_;changed=true;}
   if(ImGui::Button("Choose CSV file...")){if(auto path=dx::chooseCsvLog(csvLogPath_)){csvLogPath_=*path;std::snprintf(csvPath_,sizeof(csvPath_),"%s",path->c_str());changed=true;}}
   ImGui::TextDisabled("Press Enter to apply a typed path.");if(ImGui::InputInt("Maximum size (MB)",&csvMaxMb_)){csvMaxMb_=std::clamp(csvMaxMb_,1,4096);changed=true;}
   ImGui::TextWrapped("When the size limit is reached, the log keeps complete recent records. Saved database history remains intact.");auto status=engine_->snapshot();ImGui::TextDisabled("CSV size: %.1f MB%s",status->csvBytes/(1024.*1024.),status->csvCatchingUp?" | catching up":"");if(!status->csvError.empty())ImGui::TextWrapped("CSV log: %s",status->csvError.c_str());
   ImGui::Separator();migration();
  }else if(settingsPage_==6){
   ImGui::TextUnformatted("GridTracker");changed|=ImGui::Checkbox("Send receptions to GridTracker",&gridTrackerEnabled_);ImGui::TextWrapped("Direct UDP feed; no helper app needed. Open GridTracker, then enable this switch. Starts off.");
   ImGui::Separator();if(ImGui::InputText("Destination IP",gridTrackerAddress_,sizeof(gridTrackerAddress_),ImGuiInputTextFlags_EnterReturnsTrue)){gridTrackerHost_=gridTrackerAddress_;changed=true;}ImGui::TextDisabled("IPv4 or IPv6 address; Enter applies it.");
   if(ImGui::InputInt("UDP port",&gridTrackerPort_)){gridTrackerPort_=std::clamp(gridTrackerPort_,1,65535);changed=true;}
   ImGui::TextWrapped("Local default: 127.0.0.1, port 2237. Match GridTracker's receive address and port for another setup.");
   ImGui::Separator();changed|=ImGui::Checkbox("Replay recent receptions on reconnect",&gridTrackerReplay_);ImGui::BeginDisabled(!gridTrackerReplay_);changed|=ImGui::SliderInt("Replay window (minutes)",&gridTrackerMinutes_,1,120);ImGui::EndDisabled();ImGui::TextWrapped("Live plugin receptions from the current UTC day. Saved history and the CSV log stay intact when views are cleared.");
   auto status=engine_->snapshot()->gridTracker;ImGui::Separator();if(!status.enabled)ImGui::TextDisabled("Off");else if(!status.error.empty())ImGui::TextWrapped("GridTracker: %s",status.error.c_str());else ImGui::TextWrapped("%s | %llu receptions sent",status.lastReply&&time(nullptr)-status.lastReply<30?"GridTracker replied":"Sending UDP",status.sent);
   if(status.enabled)ImGui::TextDisabled("UDP delivery is not acknowledged by every receiver.");
   ImGui::TextWrapped("Separate frequency and mode feeds keep bands correct. This sends reception messages; it does not log completed contacts or control transmission.");
   if(ImGui::Button("Restore local defaults")){gridTrackerEnabled_=false;gridTrackerHost_="127.0.0.1";std::snprintf(gridTrackerAddress_,sizeof(gridTrackerAddress_),"%s",gridTrackerHost_.c_str());gridTrackerPort_=2237;gridTrackerReplay_=true;gridTrackerMinutes_=30;changed=true;}
  }
  ImGui::PopItemWidth();ImGui::Spacing();ImGui::Separator();ImGui::TextDisabled("Changes save automatically.");
  if(changed){tableQuery_.offset=0;save();refresh();}
  ImGui::EndChild();
 }
 ImGui::End();ImGui::PopStyleColor(2);
}
