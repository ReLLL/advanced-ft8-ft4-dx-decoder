// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <cstdio>
void DXNotebook::sharedFilters(){
 ImGui::PushID("shared_reception_filters");ImGui::Separator();
 const float available=ImGui::GetContentRegionAvail().x,gap=ImGui::GetStyle().ItemSpacing.x,padding=ImGui::GetStyle().FramePadding.x*2;
 float used=0;auto place=[&](float width){if(used&&used+gap+width<=available){ImGui::SameLine();used+=gap;}else used=0;used+=width;ImGui::SetNextItemWidth(width);};
 bool changed=false;place(125+(tableTime_==11?0:gap+ImGui::CalcTextSize("All").x+padding));ImGui::BeginGroup();ImGui::SetNextItemWidth(125);changed|=timeWindow("##time",tableTime_);ImGui::EndGroup();
 const char* bands[]={"All bands","2200m","630m","160m","80m","60m","40m","30m","20m","17m","15m","12m","10m","6m","2m","Other"};
 place(90);if(ImGui::BeginCombo("##band",tableFilter_.band.empty()?"All bands":tableFilter_.band.c_str())){for(int i=0;i<16;i++)if(ImGui::Selectable(bands[i],tableFilter_.band==(i?bands[i]:""))){tableFilter_.band=i?bands[i]:"";changed=true;}ImGui::EndCombo();}if(ImGui::IsItemHovered())ImGui::SetTooltip("Band filter for the map and all table tabs");
 const char* modes[]={"All modes","FT8","FT4","WSPR","FST4","FST4W","Q65","JS8"};
 place(90);if(ImGui::BeginCombo("##mode",tableFilter_.mode.empty()?"All modes":tableFilter_.mode.c_str())){for(int i=0;i<8;i++)if(ImGui::Selectable(modes[i],tableFilter_.mode==(i?modes[i]:""))){tableFilter_.mode=i?modes[i]:"";changed=true;}ImGui::EndCombo();}if(ImGui::IsItemHovered())ImGui::SetTooltip("Mode filter for the map and all table tabs");
 const char* kinds[]={"All messages","CQ / general call","Two-way exchange","Other / text","Beacon"};int kind=tableFilter_.cqOnly?1:int(tableFilter_.messageKind);
 place(165);if(ImGui::Combo("##message_type",&kind,kinds,5)){tableFilter_.cqOnly=false;tableFilter_.messageKind=dx::MessageKind(kind);changed=true;}if(ImGui::IsItemHovered())ImGui::SetTooltip("Message type for both views. An addressed message does not prove a completed contact.");
 const bool filtered=tableFilter_.json()!=dx::Filter{}.json(),reset=filtered||tableTime_!=11;const char* label=filtered?"Filters *":"Filters";
 const float buttonWidth=ImGui::CalcTextSize(label).x+padding,resetWidth=reset?gap+ImGui::CalcTextSize("Reset").x+padding:0;
 place(std::clamp(available-used-gap-buttonWidth-resetWidth-gap,150.f,320.f));char search[256];std::snprintf(search,sizeof(search),"%s",tableFilter_.search.c_str());if(ImGui::InputTextWithHint("##search","Search calls, countries, messages",search,sizeof(search))){tableFilter_.search=search;changed=true;}if(ImGui::IsItemHovered())ImGui::SetTooltip("Search the map and all table tabs");
 place(buttonWidth);if(ImGui::Button(label))ImGui::OpenPopup("Filter options");if(ImGui::IsItemHovered())ImGui::SetTooltip("Advanced reception filters apply to the map and all tables. An asterisk means filters are active.");
 if(reset){place(resetWidth-gap);if(ImGui::Button("Reset")){tableFilter_={};tableTime_=11;changed=true;}if(ImGui::IsItemHovered())ImGui::SetTooltip("Remove reception filters and the time limit from both views. History scope stays unchanged.");}
 ImGui::SetNextWindowSizeConstraints({420,100},{520,std::max(240.f,ImGui::GetIO().DisplaySize.y*.8f)});
 if(ImGui::BeginPopup("Filter options")){popup_=true;ImGui::TextDisabled("Map and all table tabs");ImGui::BeginChild("filter_controls",{450,std::min(570.f,ImGui::GetIO().DisplaySize.y*.7f)},false);changed|=filterEditor(tableFilter_,"table");ImGui::EndChild();ImGui::EndPopup();}
 if(changed){tableQuery_.offset=0;save();refresh();}ImGui::Separator();ImGui::PopID();
}
