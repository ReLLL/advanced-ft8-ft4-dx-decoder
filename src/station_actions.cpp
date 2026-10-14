// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <algorithm>
#include <ctime>

void DXNotebook::stationMenu(const dx::Decode& d,const std::vector<dx::Row>& rows){
 popup_=true;
 ImGui::TextDisabled("%s",d.identity.transmitter.empty()?"No transmitter identity":d.identity.transmitter.c_str());ImGui::Separator();
 if(ImGui::MenuItem("Copy original message"))ImGui::SetClipboardText(d.message.c_str());
 if(ImGui::MenuItem("Copy station JSON"))ImGui::SetClipboardText(d.json().dump(2).c_str());
 if(ImGui::MenuItem("Copy visible rows")){
  std::string all;for(const auto& row:rows){tm date{};time_t time=row.decode.utc;gmtime_r(&time,&date);char stamp[32];std::strftime(stamp,sizeof(stamp),"%Y-%m-%d %H:%M:%S",&date);all+=std::string(stamp)+" "+row.decode.identity.transmitter+" "+row.decode.message+"\n";}
  ImGui::SetClipboardText(all.c_str());
 }
 ImGui::Separator();const bool resolved=!dx::callsignLookupUrl(d.identity.transmitter,0).empty();ImGui::BeginDisabled(!resolved);
 const char* lookups[]={"Search Google","Look up on QRZ","Look up on HamQTH"};
 for(int provider=0;provider<3;provider++)if(ImGui::MenuItem(lookups[provider]))dx::openCallsignLookup(d.identity.transmitter,provider);
 ImGui::EndDisabled();
 if(ImGui::BeginMenu("Look up addressed station",!dx::callsignLookupUrl(d.identity.called,0).empty())){
  ImGui::TextDisabled("%s",d.identity.called.c_str());ImGui::Separator();
  for(int provider=0;provider<3;provider++)if(ImGui::MenuItem(lookups[provider]))dx::openCallsignLookup(d.identity.called,provider);
  ImGui::EndMenu();
 }
 ImGui::Separator();ImGui::BeginDisabled(!resolved);
 if(ImGui::MenuItem("Add favorite")){if(!watchCalls_.empty())watchCalls_+=',';watchCalls_+=d.identity.transmitter;save();}
 ImGui::EndDisabled();auto target=dx::ignoreTarget(d);
 ImGui::BeginDisabled(!target||engine_->snapshot()->moderating||ignoreWaiting_);
 if(ImGui::MenuItem("Ignore transmitter"))ignoreRequested_=target;
 ImGui::EndDisabled();
 if(target&&target->message&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("No transmitter identity: ignore this exact message across bands and modes.");
 if(ImGui::MenuItem("Unignore all")){engine_->unignoreAll();tableFilter_.excludeCall.clear();mapFilter_.excludeCall.clear();save();refresh();}
}

void DXNotebook::ignoreConfirmation(){
 auto snapshot=engine_->snapshot();const auto& ignored=ignoreTarget_.message?snapshot->ignoredMessages:snapshot->ignoredCalls;
 if(ignoreWaiting_&&!snapshot->moderating&&std::find(ignored.begin(),ignored.end(),ignoreTarget_.value)!=ignored.end())ignoreWaiting_=false;
 if(ignoreWaiting_||snapshot->moderating)ImGui::TextDisabled("Removing ignored receptions from saved logs...");
 if(ignoreRequested_){ignoreTarget_=std::move(*ignoreRequested_);ignoreRequested_.reset();ImGui::OpenPopup("Ignore transmitter and delete receptions?");}
 if(!ImGui::BeginPopupModal("Ignore transmitter and delete receptions?",nullptr,ImGuiWindowFlags_AlwaysAutoResize))return;
 popup_=true;ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+440);
 if(ignoreTarget_.message){
  ImGui::TextUnformatted("No transmitter identity is available. Delete and ignore all receptions of this exact message across bands and modes:");
  ImGui::TextWrapped("\"%s\"",ignoreTarget_.value.c_str());
  ImGui::TextUnformatted("Other unidentified messages are kept. A sender cannot be inferred from message text alone.");
 }else{
  ImGui::Text("Transmitter identity: %s",ignoreTarget_.value.c_str());
  if(ignoreTarget_.value.front()=='<')ImGui::TextUnformatted("This unresolved identifier may be reused by different stations. Matching uses the displayed identifier.");
 }
 ImGui::TextUnformatted("This blocks matching future receptions, removes matching rows and map markers, recalculates statistics, and deletes those receptions from this plugin's saved history and known reception CSV logs. Deletion cannot be undone. Uptime is kept.");ImGui::PopTextWrapPos();
 ImGui::Separator();ImGui::TextDisabled("%s",(root_/"history.sqlite3").c_str());ImGui::TextUnformatted("CSV logs to clean:");
 ImGui::BeginChild("purge_paths",{440,80},true,ImGuiWindowFlags_HorizontalScrollbar);for(const auto& path:snapshot->csvPaths)ImGui::TextUnformatted(path.c_str());ImGui::EndChild();ImGui::Separator();
 if(ImGui::Button("Ignore and delete")){if(ignoreTarget_.message)engine_->ignoreMessage(ignoreTarget_.value);else engine_->ignoreTransmitter(ignoreTarget_.value);ignoreWaiting_=true;if(selected_&&ignoreTarget_.matches(*selected_))selected_.reset();if(detailsContext_&&ignoreTarget_.matches(*detailsContext_))detailsContext_.reset();mapContext_.reset();tableQuery_.offset=0;refresh();ImGui::CloseCurrentPopup();}
 ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
