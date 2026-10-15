// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <algorithm>
#include <string>
namespace dx {
// Two seconds in, four seconds readable, two seconds out. Monotonic UI time.
inline float statusOpacity(double elapsed){
 if(elapsed<0||elapsed>=8)return 0;
 float x=float(elapsed<2?elapsed/2:elapsed<6?1:(8-elapsed)/2);
 return x*x*(3-2*x);
}
struct StatusNotification {
 std::string key,text;double started=0;
 void observe(const std::string& nextKey,const std::string& nextText,double now){
  if(nextKey==key||nextText.empty())return;
  key=nextKey;text=nextText;started=now;
 }
 float opacity(double now)const{return statusOpacity(now-started);}
};
inline float statusBarHeight(){return ImGui::GetTextLineHeight()+4;}
// Fixed item geometry, even for long errors and notifications. Full text on hover.
inline bool drawStatusBar(const std::string& message,float opacity,const std::string& activity,bool error,bool retry){
 const auto at=ImGui::GetCursorScreenPos();const float width=std::max(1.f,ImGui::GetContentRegionAvail().x),height=statusBarHeight();
 ImGui::Dummy({width,height});
 if(ImGui::IsItemHovered()&&!message.empty())ImGui::SetTooltip("%s",message.c_str());
 auto* draw=ImGui::GetWindowDrawList();const ImVec2 end{at.x+width,at.y+height};
 draw->AddRectFilled(at,end,IM_COL32(21,33,37,255),3);draw->AddLine(at,{end.x,at.y},IM_COL32(54,75,78,255));
 const float retryWidth=retry?ImGui::CalcTextSize("Retry").x+ImGui::GetStyle().FramePadding.x*2+12:0;
 const float activityWidth=std::min(width*.45f,ImGui::CalcTextSize(activity.c_str()).x);
 const float right=end.x-retryWidth-4,activityLeft=right-activityWidth;
 draw->PushClipRect({activityLeft,at.y},{right,end.y},true);
 draw->AddText({activityLeft,at.y+(height-ImGui::GetTextLineHeight())/2},IM_COL32(158,175,179,255),activity.c_str());draw->PopClipRect();
 const ImVec4 color=error?ImVec4{1,.65f,.4f,opacity}:ImVec4{.53f,.85f,.76f,opacity};
 draw->PushClipRect({at.x+4,at.y},{std::max(at.x+4,activityLeft-8),end.y},true);
 draw->AddText({at.x+4,at.y+(height-ImGui::GetTextLineHeight())/2},ImGui::ColorConvertFloat4ToU32(color),message.c_str());draw->PopClipRect();
 bool clicked=false;if(retry){auto after=ImGui::GetCursorScreenPos();ImGui::SetCursorScreenPos({end.x-retryWidth+4,at.y+2});clicked=ImGui::SmallButton("Retry");ImGui::SetCursorScreenPos(after);}
 return clicked;
}
}
