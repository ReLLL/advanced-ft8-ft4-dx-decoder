// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <cstdio>
void DXNotebook::defaultsFooter(){
 ImGui::Separator();ImGui::TextDisabled("Changes save automatically.");ImGui::SameLine();
 const float width=ImGui::CalcTextSize("Restore defaults...").x+ImGui::GetStyle().FramePadding.x*2;
 ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(),ImGui::GetCursorPosX()+ImGui::GetContentRegionAvail().x-width));
 ImGui::BeginDisabled(defaultsRequest_!=0);if(ImGui::Button("Restore defaults..."))ImGui::OpenPopup("Restore plugin defaults?");ImGui::EndDisabled();
 if(ImGui::BeginPopupModal("Restore plugin defaults?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
  popup_=true;ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+420);
  ImGui::TextUnformatted("Restore all plugin preferences, modes, alert settings, filters, table layouts, sorting and map view to their defaults?\n\nThis does not clear reception history or ignore rules. Your home-location setup is kept. Automatic retention returns to 100,000 / 95,000 database records and a 100 MB CSV limit. Oldest records beyond these limits may be removed by normal background cleanup.");ImGui::PopTextWrapPos();
  if(ImGui::Button("Restore defaults")){defaultsRequest_=engine_->restoreDefaults();ImGui::CloseCurrentPopup();}ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
 }
}
void DXNotebook::loadRestoredDefaults(const dx::Snapshot& s){
 if(!defaultsRequest_||s.settings.value("defaults_revision",uint64_t(0))!=defaultsRequest_)return;
 defaultsRequest_=0;defaultsReload_=true;settingsLoaded_=false;paused_=false;
 mapView_=MapView{};mapHomePending_=true;presets_=dx::presets();selected_.reset();selectedCountry_.clear();detailsContext_.reset();mapContext_.reset();detailsOpen_=false;detailsOffset_=0;engine_->selectCountry("");
 tableQuery_={};mapQuery_={};tableColumnsApplied_=messageColumnsApplied_=countryColumnsApplied_=tabsApplied_=false;resetTableLayouts_=15;
 customDial_[0]=0;std::snprintf(customMode_,sizeof(customMode_),"FT8");settingsError_.clear();cleanupValidation_.clear();cleanupKind_=0;cleanupCountry_.clear();cleanupFrom_[0]=cleanupTo_[0]=0;cleanupCached_=false;
 clearTarget_=0;clearStats_=true;clearRequested_=false;cleanupPreviewId_=cleanupRequestId_=0;
 settingsPage_=0;settingsPresent_=true;open_=present_=true;resetWindowGeometry_=true;
}
