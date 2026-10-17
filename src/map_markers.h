// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "modes.h"
#include <imgui.h>
namespace dx {
inline void drawModeMarker(ImDrawList* draw,std::string_view mode,ImVec2 center,float radius,ImU32 color,bool filled=true){
 auto shape=modeMarker(mode);
 if(shape==MarkerShape::Circle){if(filled)draw->AddCircleFilled(center,radius,color);else draw->AddCircle(center,radius,color,16,1.8f);return;}
 auto outline=markerOutline(shape,radius);ImVec2 points[8];for(int i=0;i<outline.count;i++)points[i]={center.x+outline.points[i][0],center.y+outline.points[i][1]};
 if(filled)draw->AddConvexPolyFilled(points,outline.count,color);else draw->AddPolyline(points,outline.count,color,ImDrawFlags_Closed,1.8f);
}
}
