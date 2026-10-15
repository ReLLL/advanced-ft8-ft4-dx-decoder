// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui_internal.h>
namespace dx {
// Schedule before BeginTable: initialization there resets column memory.
// Calling TableResetSettings after BeginTable leaves old positive widths intact.
inline void resetTableLayout(const char* name,unsigned int& pending,unsigned int bit){
 if(!(pending&bit))return;const auto id=ImGui::GetID(name);
 if(auto* table=ImGui::TableFindByID(id))table->IsResetAllRequest=true;
 else if(auto* settings=ImGui::TableSettingsFindByID(id))settings->ID=0;
 pending&=~bit;
}
}
