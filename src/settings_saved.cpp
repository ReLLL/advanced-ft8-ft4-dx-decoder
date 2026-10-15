// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
bool DXNotebook::historyDatabaseSettings(){
 bool changed=false;
 auto snapshot=engine_->snapshot();
 ImGui::Text("Saved receptions: %lld",static_cast<long long>(snapshot->savedMessages));
 if(snapshot->sqliteBytes){ImGui::SameLine();ImGui::Text("| SQLite file: %.1f MB",double(*snapshot->sqliteBytes)/1000000.);}
 else {ImGui::SameLine();ImGui::TextDisabled("| SQLite file: --");}
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Main reception database file; 1 MB = 1,000,000 bytes.\nThe active SQLite journal uses additional disk space.\nUpdated in the background; independent of table/map filters.");ImGui::Spacing();
 ImGui::TextUnformatted("Automatic retention");changed|=ImGui::Checkbox("Automatically limit saved database history",&historyAutoTrim_);ImGui::BeginDisabled(!historyAutoTrim_);
 ImGui::SetNextItemWidth(135);if(ImGui::InputInt("Start cleanup above",&historyMaxRecords_,1000,10000,ImGuiInputTextFlags_EnterReturnsTrue)){historyMaxRecords_=std::clamp(historyMaxRecords_,10000,1000000);historyTrimTo_=std::clamp(historyTrimTo_,1,historyMaxRecords_-1);changed=true;}
 ImGui::SetNextItemWidth(135);if(ImGui::InputInt("Keep newest after cleanup",&historyTrimTo_,1000,10000,ImGuiInputTextFlags_EnterReturnsTrue)){historyTrimTo_=std::clamp(historyTrimTo_,1,historyMaxRecords_-1);changed=true;}
 ImGui::TextDisabled("Press Enter to apply a typed count.");
 ImGui::EndDisabled();ImGui::TextWrapped("Record counts. Defaults: start above 100,000; keep the newest 95,000. Checked every 30 minutes, at startup and after changing these settings. Oldest records are removed in small background batches. New receptions are protected. Higher limits can slow queries. CSV has a separate size limit under History.");
 ImGui::TextDisabled("Database and journal: %.1f MB | checked every 30 minutes",snapshot->historyBytes/(1024.*1024.));
 if(snapshot->retention.firstUtc)ImGui::TextWrapped("Database history since %s%s",dx::localDateTime(snapshot->retention.firstUtc).c_str(),snapshot->retention.running?" | cleaning older records":"");
 if(snapshot->retention.deleted)ImGui::TextDisabled("Automatically deleted: %lld older receptions",static_cast<long long>(snapshot->retention.deleted));
 if(!snapshot->retentionError.empty())ImGui::TextWrapped("Automatic cleanup will retry: %s",snapshot->retentionError.c_str());
 ImGui::Spacing();ImGui::Separator();ImGui::TextUnformatted("One-off cleanup");
 ImGui::TextWrapped("Manage this plugin's SQLite reception history and all known reception CSV logs together. Each cleanup runs once, independently of table/map filters. New receptions remain allowed.");ImGui::Spacing();
 const char* operations[]={"Delete before a date","Delete after a date","Delete between two dates","Delete selected country","Keep only selected country","Delete records with no distance","Delete records with no grid"};
 if(ImGui::Combo("Operation",&cleanupKind_,operations,7))cleanupValidation_.clear();
 if(cleanupKind_<=2){
  ImGui::InputTextWithHint(cleanupKind_==2?"Start date":"Date","YYYY-MM-DD or YYYY-MM-DD HH:MM:SS",cleanupFrom_,sizeof(cleanupFrom_));
  if(cleanupKind_==2)ImGui::InputTextWithHint("End date","YYYY-MM-DD or YYYY-MM-DD HH:MM:SS",cleanupTo_,sizeof(cleanupTo_));
  ImGui::TextWrapped("Dates use the computer's time zone. A date alone means midnight for Before/Start, and the end of that day for After/End. Between includes both endpoints.");
 }else if(cleanupKind_==3||cleanupKind_==4){
  if(cleanupCountry_.empty()&&selected_&&!selected_->country.empty())cleanupCountry_=selected_->country;
  if(ImGui::BeginCombo("Country",cleanupCountry_.empty()?"Choose a country":cleanupCountry_.c_str())){for(const auto& country:snapshot->savedCountries)if(ImGui::Selectable(country.c_str(),cleanupCountry_==country))cleanupCountry_=country;ImGui::EndCombo();}
  if(cleanupKind_==4)ImGui::TextWrapped("Keep the selected country's saved receptions. Delete every other country, including unidentified countries.");
 }else if(cleanupKind_==5){
  ImGui::Checkbox("Include cached-grid distances",&cleanupCached_);
  ImGui::TextWrapped(cleanupCached_?"Delete missing distances and distances inferred from a previously received grid.":"Delete missing numeric distances. Keep distances based on cached grids.");
 }else ImGui::TextWrapped("Delete receptions whose saved grid field is empty. Receptions with a received or cached grid are kept.");
 ImGui::Spacing();ImGui::BeginDisabled(snapshot->cleanupBusy||snapshot->moderating);
 if(ImGui::Button("Review deletion...")){
  dx::SavedCleanup criteria;criteria.kind=dx::CleanupKind(cleanupKind_);criteria.country=cleanupCountry_;criteria.includeCached=cleanupCached_;cleanupValidation_.clear();
  if(cleanupKind_<=2){auto from=dx::cleanupDate(cleanupFrom_,cleanupKind_==1);auto to=dx::cleanupDate(cleanupTo_,true);if(from)criteria.from=*from;if(to)criteria.to=*to;}
  if(!criteria.valid())cleanupValidation_=cleanupKind_<=2?"Enter valid local dates; the end must not precede the start. Times skipped or repeated by daylight-saving changes are not accepted.":"Choose a country.";
  else {cleanupCriteria_=criteria;cleanupPreviewId_=engine_->previewCleanup(criteria);ImGui::OpenPopup("Delete saved messages?");}
 }
 ImGui::EndDisabled();if(!cleanupValidation_.empty())ImGui::TextColored({1,.65f,.4f,1},"%s",cleanupValidation_.c_str());
 if(snapshot->cleanupBusy)ImGui::TextWrapped("Updating saved messages, filtering CSV logs and compacting SQLite in the background...");
 if(!snapshot->cleanupError.empty())ImGui::TextColored({1,.65f,.4f,1},"Cleanup pending: %s. It will retry automatically.",snapshot->cleanupError.c_str());
 else if(cleanupRequestId_&&snapshot->completedCleanup>=cleanupRequestId_&&!snapshot->cleanupBusy)ImGui::TextWrapped("Cleanup complete: %lld database receptions deleted; logs updated and database compacted.",static_cast<long long>(snapshot->cleanupDeleted));
 if(ImGui::BeginPopupModal("Delete saved messages?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
  popup_=true;ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+460);ImGui::TextWrapped("%s",cleanupCriteria_.description().c_str());
  bool ready=snapshot->cleanupPreviewId==cleanupPreviewId_&&snapshot->cleanupPreview==cleanupCriteria_.json();
  if(ready)ImGui::Text("Database matches: %lld receptions",static_cast<long long>(snapshot->cleanupMatches));else ImGui::TextDisabled("Counting matching saved receptions...");
  ImGui::TextUnformatted("Apply this rule to saved SQLite history and known CSV logs, remove matching entries from map/table, recalculate saved statistics and vacuum the database. Deletion cannot be undone. No ignore rules are added. Receptions received after this preview are kept.");ImGui::PopTextWrapPos();
  ImGui::BeginChild("cleanup_files",{460,90},true,ImGuiWindowFlags_HorizontalScrollbar);ImGui::TextUnformatted((root_/"history.sqlite3").c_str());for(const auto& path:snapshot->csvPaths)ImGui::TextUnformatted(path.c_str());ImGui::EndChild();ImGui::Separator();
  ImGui::BeginDisabled(!ready||snapshot->cleanupBusy||snapshot->moderating);if(ImGui::Button("Delete matching records")){cleanupRequestId_=engine_->deleteSaved(cleanupCriteria_,snapshot->cleanupThroughId);selected_.reset();selectedCountry_.clear();detailsContext_.reset();mapContext_.reset();tableQuery_.offset=0;engine_->selectCountry("");refresh();ImGui::CloseCurrentPopup();}ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
 }
 return changed;
}
