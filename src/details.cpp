// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <ctime>
#include <cstdio>
namespace {
const ImVec4 senderColor{.66f,.82f,1,1},countryColor{.63f,.89f,.66f,1},distanceColor{1,.76f,.55f,1},modeColor{.52f,.84f,.82f,1},messageColor{.92f,.94f,.95f,1};
ImVec4 signalColor(int snr){auto color=dx::snrMapColor(snr);return {color[0],color[1],color[2],color[3]};}
void field(const char* label,const std::string& value,const ImVec4& color){ImGui::TableNextColumn();ImGui::TextDisabled("%s",label);ImGui::TextColored(color,"%s",value.c_str());}
void right(const char* text,const ImVec4& color){ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(text).x));ImGui::TextColored(color,"%s",text);}
}
void DXNotebook::details(){
 auto snapshot=engine_->snapshot();const bool countryView=!selectedCountry_.empty();ImGui::Separator();ImGui::BeginChild("station_details",{0,0},false);
 if(ImGui::BeginTable("details_heading",2,ImGuiTableFlags_SizingStretchProp)){
  ImGui::TableSetupColumn("Station",ImGuiTableColumnFlags_WidthStretch);ImGui::TableSetupColumn("Close",ImGuiTableColumnFlags_WidthFixed,ImGui::CalcTextSize("Close details").x+ImGui::GetStyle().FramePadding.x*2);
  ImGui::TableNextColumn();
  if(!countryView){const auto& d=*selected_;ImGui::TextColored(senderColor,"%s",d.identity.transmitter.empty()?"Unresolved sender":d.identity.transmitter.c_str());ImGui::SameLine(0,14);}
  const auto& name=countryView?selectedCountry_:selected_->country;countryFlag(name);ImGui::TextColored(countryColor,"%s",name.empty()?"Unknown country":name.c_str());
  ImGui::TableNextColumn();if(ImGui::SmallButton("Close details")){ImGui::EndTable();ImGui::EndChild();selected_.reset();selectedCountry_.clear();engine_->selectCountry("");save();return;}ImGui::EndTable();
 }
 if(countryView)ImGui::TextDisabled("All matching receptions | newest first | repeats included");
 else{
  const auto& d=*selected_;
  if(ImGui::BeginTable("details_fields",3,ImGuiTableFlags_SizingStretchSame)){
   auto target=dx::targetCall(d.identity);field("Target",target.empty()?(dx::messageKind(d.identity)==dx::MessageKind::GeneralCall?"General call":"--"):target,senderColor);
   char number[96];if(d.km){std::snprintf(number,sizeof(number),"%s%s",d.gridSource=="cached"?"* ":"",dx::distanceSummary(*d.km,distanceMiles_).c_str());std::string value=number;if(d.bearing){std::snprintf(number,sizeof(number)," | %.0f deg | %s",std::fmod(std::round(*d.bearing),360.),dx::compassDirection(*d.bearing).c_str());value+=number;}field("Distance / bearing",value,distanceColor);}else field("Distance / bearing",std::string("-- ")+dx::distanceUnit(distanceMiles_)+" (no received grid)",distanceColor);
   std::snprintf(number,sizeof(number),"%+d dB SNR | %d Hz audio",d.snr,d.audio);field("Received signal",number,signalColor(d.snr));
   field("Received",dx::localDateTime(d.utc),messageColor);std::snprintf(number,sizeof(number),"%s | %s | %.6f MHz",d.mode.c_str(),dx::bandName(d.dial).c_str(),d.dial/1e6);field("Mode / band / dial",number,modeColor);
   std::string grid=d.identity.grid.empty()?"--":d.identity.grid;if(d.dt){std::snprintf(number,sizeof(number)," | DT %+.1f s",*d.dt);grid+=number;}field("Grid / timing",grid,modeColor);ImGui::EndTable();
  }
  ImGui::PushTextWrapPos(0);ImGui::TextColored(messageColor,"\"%s\"",d.message.c_str());ImGui::PopTextWrapPos();
  ImGui::TextDisabled("Source: %s | Identity: %s | Grid: %s",d.source.c_str(),d.identitySource.c_str(),d.gridSource.empty()?"unavailable":d.gridSource.c_str());ImGui::Separator();
 }
 ImGui::TextDisabled("Distances: %s",dx::distanceUnit(distanceMiles_));
 const auto menuId=ImGui::GetID("details_station_actions");
 if(snapshot->detailsCountry!=selectedCountry_){ImGui::TextDisabled("Loading receptions...");ImGui::EndChild();return;}
 if(snapshot->details.empty())ImGui::TextDisabled("No matching receptions in this view.");
 else if(ImGui::BeginTable("details_history",10,ImGuiTableFlags_ScrollX|ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg|ImGuiTableFlags_Resizable,{0,std::max(55.f,ImGui::GetContentRegionAvail().y-(snapshot->details.size()==500?30.f:0.f))})){
  const char* labels[]={"Date/time","Sender","Target","Country","Grid","SNR dB","Distance","Mode / band","Message","Source"};const float widths[]={205,95,95,150,60,65,90,85,260,70};for(int i=0;i<10;i++)ImGui::TableSetupColumn(labels[i],ImGuiTableColumnFlags_WidthFixed,widths[i]);ImGui::TableSetupScrollFreeze(0,1);ImGui::TableHeadersRow();
  ImGuiListClipper clipper;clipper.Begin(int(snapshot->details.size()));while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;i++){
   const auto& h=snapshot->details[size_t(i)];ImGui::PushID(i);ImGui::TableNextRow();ImGui::TableNextColumn();auto at=ImGui::GetCursorPos();ImGui::Selectable("##details_row",false,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowItemOverlap);
   if(ImGui::IsItemHovered()&&ImGui::IsMouseReleased(1)){detailsContext_=h;ImGui::OpenPopup(menuId);}ImGui::SetCursorPos(at);ImGui::TextDisabled("%s",dx::localDateTime(h.utc).c_str());
   ImGui::TableNextColumn();ImGui::TextColored(senderColor,"%s",h.identity.transmitter.empty()?"--":h.identity.transmitter.c_str());ImGui::TableNextColumn();auto target=dx::targetCall(h.identity);ImGui::TextColored(senderColor,"%s",target.empty()?"--":target.c_str());
   ImGui::TableNextColumn();countryFlag(h.country);ImGui::TextColored(countryColor,"%s",h.country.empty()?"Unknown":h.country.c_str());ImGui::TableNextColumn();ImGui::TextColored(modeColor,"%s",h.identity.grid.c_str());
   ImGui::TableNextColumn();char number[32];std::snprintf(number,sizeof(number),"%+d",h.snr);right(number,signalColor(h.snr));if(ImGui::IsItemHovered())ImGui::SetTooltip("Receiver SNR: %+d dB | Audio: %d Hz | DT: %+.1f s",h.snr,h.audio,h.dt.value_or(0));
   ImGui::TableNextColumn();if(h.km)std::snprintf(number,sizeof(number),"%s%.0f",h.gridSource=="cached"?"* ":"",dx::distanceDisplay(*h.km,distanceMiles_));else std::snprintf(number,sizeof(number),"--");right(number,distanceColor);
   ImGui::TableNextColumn();ImGui::TextColored(modeColor,"%s / %s",h.mode.c_str(),dx::bandName(h.dial).c_str());ImGui::TableNextColumn();ImGui::TextColored(messageColor,"\"%s\"",h.message.c_str());if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",h.message.c_str());
   ImGui::TableNextColumn();ImGui::TextDisabled("%s",h.source.c_str());ImGui::PopID();
  }ImGui::EndTable();
 }
 if(ImGui::BeginPopup("details_station_actions")){if(detailsContext_){std::vector<dx::Row> visible;for(const auto& reception:snapshot->details)visible.push_back({reception});stationMenu(*detailsContext_,visible);}ImGui::EndPopup();}
 if(snapshot->details.size()==500&&ImGui::Button("Older messages")){detailsOffset_+=500;if(countryView)engine_->selectCountry(selectedCountry_,detailsOffset_);else engine_->select(*selected_,detailsOffset_);}ImGui::EndChild();
}
