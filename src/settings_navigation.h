// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
namespace dx {
inline constexpr const char* settingsPages[]={"General","Reception","Map","Alerts","Filters","History","Integrations","History DB"};
inline int settingsNavigation(int page){
 for(int i=0;i<5;i++)if(ImGui::Selectable(settingsPages[i],page==i))page=i;
 ImGui::SetNextItemOpen(true,ImGuiCond_Once);
 const bool open=ImGui::TreeNodeEx("History",ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_OpenOnDoubleClick|ImGuiTreeNodeFlags_SpanAvailWidth|(page==5?ImGuiTreeNodeFlags_Selected:0));
 if(ImGui::IsItemClicked()&&!ImGui::IsItemToggledOpen())page=5;
 if(open){if(ImGui::Selectable("History DB",page==7))page=7;ImGui::TreePop();}
 if(ImGui::Selectable("Integrations",page==6))page=6;
 return page;
}
}
