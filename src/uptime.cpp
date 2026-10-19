// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <algorithm>
#include <ctime>
void DXNotebook::uptimeStat(const dx::Snapshot& snapshot){
 const auto active=dx::durationText(int64_t(snapshot.stats.activeSeconds));
 const auto span=dx::historySpanText(snapshot.statsHistory||snapshot.historyAll,snapshot.retention.firstUtc,time(nullptr));
 const float font=ImGui::GetFontSize(),width=ImGui::GetContentRegionAvail().x,height=font*2+ImGui::GetStyle().ItemSpacing.y;
 auto pos=ImGui::GetCursorScreenPos();auto* draw=ImGui::GetWindowDrawList();
 const auto muted=IM_COL32(158,171,179,255),accent=IM_COL32(94,201,178,255),history=IM_COL32(218,183,116,255);
 draw->PushClipRect(pos,{pos.x+width,pos.y+height},true);draw->AddText(pos,muted,"UPTIME");
 const auto prefix=span.empty()?active:active+" (  ";
 const auto suffix=span.empty()?std::string{}:span+")";
 const float needed=ImGui::CalcTextSize((prefix+suffix).c_str()).x+(span.empty()?0:font*.9f);
 const float scale=needed>0?std::clamp(width/needed,.75f,1.f):1.f,size=font*scale;
 const ImVec2 value{pos.x,pos.y+font+ImGui::GetStyle().ItemSpacing.y};draw->AddText(ImGui::GetFont(),size,value,accent,active.c_str());
 if(!span.empty()){
  float x=value.x+ImGui::CalcTextSize((active+" ").c_str()).x*scale;draw->AddText(ImGui::GetFont(),size,{x,value.y},history,"(");x+=ImGui::CalcTextSize("(").x*scale;
  // Draw the clock locally: both hosts' default font atlases omit Unicode clock glyphs.
  const float radius=size*.32f;const ImVec2 center{x+radius+size*.15f,value.y+size*.52f};
  draw->AddCircle(center,radius,history,16,1.f);draw->AddLine(center,{center.x,center.y-radius*.65f},history,1.f);draw->AddLine(center,{center.x+radius*.55f,center.y},history,1.f);
  x+=size*.95f;draw->AddText(ImGui::GetFont(),size,{x,value.y},history,suffix.c_str());
 }
 draw->PopClipRect();ImGui::Dummy({width,height});
 if(ImGui::IsItemHovered()){
  ImGui::BeginTooltip();ImGui::PushTextWrapPos(380);ImGui::Text("Active decoding this session: %s",active.c_str());
  if(!span.empty()){ImGui::TextColored({.855f,.718f,.455f,1},"History elapsed: %s",span.c_str());ImGui::Text("Oldest saved reception: %s",dx::localDateTime(snapshot.retention.firstUtc).c_str());ImGui::TextUnformatted("The amber clock value in parentheses runs from the oldest retained reception to now, including gaps and stopped periods. It covers the full retained database, independent of map/table filters. Deleting or automatically trimming the oldest records moves its start date forward.");}
  ImGui::PopTextWrapPos();ImGui::EndTooltip();
 }
}
