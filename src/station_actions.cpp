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
  std::string all;for(const auto& row:rows){all+=dx::localDateTime(row.decode.utc)+" "+row.decode.identity.transmitter+" "+row.decode.message+"\n";}
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
 if(ImGui::MenuItem("Ignore transmitter")){deletePrimary_=false;ignoreRequested_=target;}
 if(ImGui::MenuItem("Delete transmitter")){deletePrimary_=true;ignoreRequested_=target;}
 ImGui::EndDisabled();
 if(target&&target->message&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("No transmitter identity: this action matches the exact message across bands and modes.");
 if(ImGui::MenuItem("Unignore all")){engine_->unignoreAll();tableFilter_.excludeCall.clear();save();refresh();}
}

void DXNotebook::ignoreConfirmation(){
 auto snapshot=engine_->snapshot();
 if(ignoreWaiting_&&snapshot->completedModeration>=ignoreRequestId_&&!snapshot->moderating)ignoreWaiting_=false;
 if(ignoreWaiting_||snapshot->moderating)ImGui::TextDisabled("Applying transmitter action...");
 const char* title=deletePrimary_?"Delete transmitter?":"Ignore transmitter?";
 if(ignoreRequested_){ignoreTarget_=std::move(*ignoreRequested_);ignoreRequested_.reset();alsoAction_=false;ImGui::OpenPopup(title);}
 if(!ImGui::BeginPopupModal(title,nullptr,ImGuiWindowFlags_AlwaysAutoResize))return;
 popup_=true;ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+440);
 if(ignoreTarget_.message){
  ImGui::TextUnformatted("No transmitter identity is available. This action matches this exact message across bands and modes:");
  ImGui::TextWrapped("\"%s\"",ignoreTarget_.value.c_str());
  ImGui::TextUnformatted("Other unidentified messages are kept. A sender cannot be inferred from message text alone.");
 }else{
  ImGui::Text("Transmitter identity: %s",ignoreTarget_.value.c_str());
  if(ignoreTarget_.value.front()=='<')ImGui::TextUnformatted("This unresolved identifier may be reused by different stations. Matching uses the displayed identifier.");
 }
 ImGui::Checkbox(deletePrimary_?"Also ignore future receptions":"Also delete saved receptions",&alsoAction_);
 bool deleteRecords=deletePrimary_||alsoAction_,ignoreFuture=!deletePrimary_||alsoAction_;
 ImGui::TextUnformatted(ignoreFuture?"Block matching future receptions on every band and mode.":"Future receptions can appear again. This action adds no ignore rule.");
 if(deleteRecords)ImGui::TextUnformatted("Delete matching receptions from tables, map, statistics, SQLite history and known CSV logs. Deletion cannot be undone. Uptime is kept.");
 else ImGui::TextUnformatted("Keep existing table/map entries, statistics and saved reception logs.");
 const auto& ignored=ignoreTarget_.message?snapshot->ignoredMessages:snapshot->ignoredCalls;
 if(!ignoreFuture&&std::find(ignored.begin(),ignored.end(),ignoreTarget_.value)!=ignored.end())ImGui::TextUnformatted("This identity is already ignored. Its existing ignore rule stays in place.");
 ImGui::PopTextWrapPos();
 if(deleteRecords){ImGui::Separator();ImGui::TextDisabled("%s",(root_/"history.sqlite3").c_str());ImGui::TextUnformatted("CSV logs to clean:");ImGui::BeginChild("purge_paths",{440,80},true,ImGuiWindowFlags_HorizontalScrollbar);for(const auto& path:snapshot->csvPaths)ImGui::TextUnformatted(path.c_str());ImGui::EndChild();}
 ImGui::Separator();const char* action=deleteRecords&&ignoreFuture?"Delete and ignore":deleteRecords?"Delete transmitter":"Ignore transmitter";
 if(ImGui::Button(action)){ignoreRequestId_=engine_->moderate(ignoreTarget_,deleteRecords,ignoreFuture);ignoreWaiting_=ignoreRequestId_>0;if(deleteRecords){if(selected_&&ignoreTarget_.matches(*selected_))selected_.reset();if(detailsContext_&&ignoreTarget_.matches(*detailsContext_))detailsContext_.reset();mapContext_.reset();tableQuery_.offset=0;}refresh();ImGui::CloseCurrentPopup();}
 ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
