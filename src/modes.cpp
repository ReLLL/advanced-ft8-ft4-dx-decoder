// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "modes.h"
#include <cmath>
namespace dx {
const std::array<ModeSpec,6>& modeSpecs(){
 static const std::array<ModeSpec,6> all={{
  {"FT8",MarkerShape::Circle,false,true,{15000},1},
  {"FT4",MarkerShape::Circle,false,true,{7500},1},
  {"WSPR",MarkerShape::Square,true,bool(DX_ADDITIONAL_MODES),{120000},1},
  {"FST4",MarkerShape::Triangle,false,bool(DX_ADDITIONAL_MODES),{15000,30000,60000,120000,300000,900000,1800000},7},
  {"FST4W",MarkerShape::Diamond,true,bool(DX_ADDITIONAL_MODES),{120000,300000,900000,1800000},4},
  {"Q65",MarkerShape::Hexagon,false,bool(DX_ADDITIONAL_MODES),{15000,30000,60000,120000,300000},5}
 }};return all;
}
const ModeSpec* modeSpec(std::string_view name){for(const auto& mode:modeSpecs())if(mode.name==name)return &mode;return nullptr;}
std::optional<int64_t> capturePeriodMs(std::string_view name,int requestedMs){
 auto mode=modeSpec(name);if(!mode)return {};if(!requestedMs)return mode->periodsMs[0];
 for(size_t i=0;i<mode->periodCount;i++)if(mode->periodsMs[i]==requestedMs)return requestedMs;return {};
}
MarkerShape modeMarker(std::string_view name){auto mode=modeSpec(name);return mode?mode->marker:MarkerShape::Circle;}
std::vector<int> sequencePeriods(std::string_view name,int selectionMs){
 auto mode=modeSpec(name);if(!mode)return {};if(selectionMs)return capturePeriodMs(name,selectionMs)?std::vector<int>{selectionMs}:std::vector<int>{};
 return {mode->periodsMs.begin(),mode->periodsMs.begin()+mode->periodCount};
}
int sequenceSubmodeMaximum(int selectionMs){return selectionMs==0||selectionMs==15000?2:selectionMs==30000?3:4;}
MarkerOutline markerOutline(MarkerShape shape,float radius){
 MarkerOutline out;if(!std::isfinite(radius)||radius<=0)return out;
 constexpr double pi=3.14159265358979323846;double start=-pi/2;
 switch(shape){case MarkerShape::Circle:return out;case MarkerShape::Square:out.count=4;start=pi/4;break;case MarkerShape::Triangle:out.count=3;break;case MarkerShape::Diamond:out.count=4;break;case MarkerShape::Hexagon:out.count=6;start=0;break;}
 for(int i=0;i<out.count;i++){double angle=start+2*pi*i/out.count;out.points[i]={float(radius*std::cos(angle)),float(radius*std::sin(angle))};}return out;
}
}
