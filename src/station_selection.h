// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <algorithm>
namespace dx {
inline int keyboardStationRow(int selected,int count){
 if(count<=0||!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)||ImGui::GetIO().WantTextInput)return selected;
 // Hosts need not enable global ImGui navigation; keep arrows local to this table.
 if(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_NavEnableKeyboard)return selected;
 if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))return std::min(selected+1,count-1);
 if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))return std::max(0,selected-1);
 return selected;
}
struct StationRowAction {bool select=false,openDetails=false;};
inline StationRowAction stationRowSelection(const char* label,bool selected){
 bool activated=ImGui::Selectable(label,selected,ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowDoubleClick);
 bool navigated=(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_NavEnableKeyboard)&&ImGui::IsItemFocused()&&!selected;
 return {activated||navigated,activated&&ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0)};
}
}
