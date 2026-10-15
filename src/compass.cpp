// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <cstdio>
void DXNotebook::directionStat(const char* value,std::optional<double> bearing){
 const float font=ImGui::GetFontSize(),height=font*2+ImGui::GetStyle().ItemSpacing.y,width=ImGui::GetContentRegionAvail().x,diameter=height;
 auto pos=ImGui::GetCursorScreenPos();auto* draw=ImGui::GetWindowDrawList();const float textWidth=std::max(0.f,width-diameter-7),radius=diameter/2;
 const ImVec2 center{pos.x+width-radius,pos.y+radius};const auto muted=IM_COL32(158,171,179,255),accent=IM_COL32(94,201,178,255);
 draw->PushClipRect(pos,{pos.x+textWidth,pos.y+height},true);
 const char* label=ImGui::CalcTextSize("DX ACTIVITY BEARING").x*.78f<=textWidth?"DX ACTIVITY BEARING":"DX BEARING";
 draw->AddText(ImGui::GetFont(),font*.78f,pos,muted,label);std::string shown=value;
 if(ImGui::CalcTextSize(value).x>textWidth){if(bearing){char degrees[24];std::snprintf(degrees,sizeof(degrees),"%.0f°",std::fmod(std::round(*bearing),360.));shown=degrees;}else shown=std::string(value).find("Mixed")==0?"Mixed":"Waiting";}
 draw->AddText({pos.x,pos.y+font+ImGui::GetStyle().ItemSpacing.y},accent,shown.c_str());draw->PopClipRect();
 draw->AddCircleFilled(center,radius-1,IM_COL32(12,19,23,255),40);draw->AddCircle(center,radius-1,IM_COL32(138,155,163,230),40,1.1f);draw->AddCircle(center,radius-3,IM_COL32(55,72,79,230),40,.65f);
 constexpr double rad=3.14159265358979323846/180;
 for(int i=0;i<16;i++){double a=i*22.5*rad;float outer=radius-3,inner=outer-(i%4?1.4f:2.8f);draw->AddLine({center.x+float(std::sin(a))*inner,center.y-float(std::cos(a))*inner},{center.x+float(std::sin(a))*outer,center.y-float(std::cos(a))*outer},muted,.75f);}
 const float northSize=std::max(6.f,font*.52f);draw->AddText(ImGui::GetFont(),northSize,{center.x-northSize*.3f,center.y-radius+4},IM_COL32(233,203,195,255),"N");
 if(bearing&&std::isfinite(*bearing)){const double angle=*bearing*rad;const ImVec2 direction{float(std::sin(angle)),float(-std::cos(angle))},normal{-direction.y,direction.x};float length=radius*.6f,half=radius*.17f;
  ImVec2 left{center.x+normal.x*half,center.y+normal.y*half},right{center.x-normal.x*half,center.y-normal.y*half};draw->AddTriangleFilled({center.x+direction.x*length,center.y+direction.y*length},left,right,IM_COL32(229,76,72,255));draw->AddTriangleFilled({center.x-direction.x*length,center.y-direction.y*length},right,left,IM_COL32(238,238,224,255));
 }
 draw->AddCircleFilled(center,1.8f,IM_COL32(158,170,173,255),12);draw->AddCircleFilled({center.x-.45f,center.y-.5f},.7f,IM_COL32(237,242,242,255),8);
 ImGui::Dummy({width,height});
}
