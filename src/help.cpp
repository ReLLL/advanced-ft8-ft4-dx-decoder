// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"

void DXNotebook::help(){
 ImGui::SetNextWindowSize({720,620},ImGuiCond_FirstUseEver);
 if(!ImGui::BeginPopupModal("Decoder help",nullptr,ImGuiWindowFlags_None))return;
 popup_=true;
 ImGui::BeginChild("help_content",{0,-ImGui::GetFrameHeightWithSpacing()-8},false);
 if(ImGui::CollapsingHeader("Window, tabs and waterfall corner",ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s","Closing this window hides the interface. Reception and logging continue while the sidebar switch is enabled, the SDR is running and reception is not paused. Open brings the window forward with current data and keeps its size and position. Closing does not reset your data.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Unique stations shows the latest matching reception per sender. All messages includes every retained repeat, newest UTC first, and adds UTC and source columns. Countries heard counts distinct senders and averages their latest matching receptions once. Missing distances are excluded; average SNR is an arithmetic mean in dB, not combined signal power. Each table remembers its layout and sort; filters are shared. Exports follow the active tab.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Corner notifications on waterfall starts on in General settings. Each new reception appears with its flag, country, sender, distance, receiver SNR and full italic message, including repeats. Rows glow for two seconds and expire after an adjustable 10 seconds. The farthest reception stays below with a trophy and flag; only Farthest is gold. Country text is green, sender blue and distance orange. Saved history does not replay old notifications. Long lists/messages scroll. Unmapped regions use a neutral flag.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Country names have flags in every table and in details. Double-click a Countries heard row for all its matching messages, newest first, including repeats. Table filters and time/history scope apply. Details use separate field colors and show sender, target, UTC, signal, distance, grid, mode, message and source. Distance units in General settings defaults to kilometers; miles is optional and remembered. Displays and distance filters follow the selected unit; logs and exports retain named kilometer fields.");
 }
 if(ImGui::CollapsingHeader("First-heard times",ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s","One table row per callsign across matching bands and modes. Message, SNR and other reception fields update from the latest matching reception. Equal timestamps use the last arrival. All receptions remain in saved logs and station details.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Msg age shows how long ago the latest reception occurred. A repeated message refreshes that value.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Msg first heard is the first reception of the exact message from the same callsign in this session. Repeats keep that timestamp. A different message has its own first-heard time.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Station first heard is the first reception of any message from that callsign in this session, across all bands and modes.");ImGui::Spacing();
  ImGui::TextWrapped("%s","All age displays show seconds followed by ago through 600 seconds, then the original UTC date and time. Sorting uses the underlying timestamp regardless of the displayed format. Hover always shows the original UTC date and time. By default, Brown restart starts both again with new receptions. Use saved history for everything restores the first retained reception times. Click a header to sort by its timestamp.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Columns and filters",0)){
  ImGui::TextWrapped("%s","Sender is the transmitting callsign. Target is the addressed callsign, or -- for a general call or no target. Mod shows the latest FT8/FT4 mode. Target and Mod initially appear last. Drag headers to reorder columns; drag edges to resize. Right-click a header to show or hide columns. Order, widths, visibility and sorting are saved across restarts.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Filters cover all columns. Text, band and mode filters select matching receptions first. Column value ranges then test the displayed rows: SNR, distance, dial MHz, audio Hz, signed DT, bearing, reception count and the three ages. Ages use seconds ago and refresh with time. Limits are inclusive; unknown values do not match enabled limits. A range never replaces the latest row with an older decode.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Distances and bearing",ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s","Distances use the configured home grid center and the received station grid center. Changing home recalculates saved distances in the background. Map hover shows the grid and coordinates used.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Distances are approximate great-circle distances from Brown's home Maidenhead grid center to the station's received grid center. A four-character grid covers an area, so its center is not an exact transmitter location.");ImGui::Spacing();
  ImGui::TextWrapped("%s","An asterisk marks a cached grid received within the preceding six hours. Repeated messages without a grid do not renew the evidence. No recent received grid means unknown distance. Callsign prefixes identify a DXCC entity, not a precise station location.");ImGui::Spacing();
  ImGui::TextWrapped("%s","The legacy decoder can report a different distance because it uses a country or regional reference point. Our distances use received grid evidence.");ImGui::Spacing();
  ImGui::TextWrapped("%s","DX ACTIVITY BEARING always uses receptions within the last 15 minutes, regardless of older history or map/table time windows. The station count is adjustable. DX activity is the circular mean of geographic short-path bearings to the farthest recent unique stations, measured from home. It describes heard activity, not measured arrival direction or a propagation forecast. Mixed directions means the bearings are too dispersed for one useful direction.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Table colors",0)){
  ImGui::TextWrapped("%s","Green country: every displayed station row from one of the three most recently discovered countries. The color stays through message updates. A fourth new country returns the oldest country to normal text. Repeats and other stations from an already heard country do not renew its position. Discovery uses this session, or all retained receptions when saved history is used for everything. Gold bold country and distance mark the farthest station in the selected statistics scope; gold takes precedence over green.");ImGui::Spacing();
  ImGui::TextWrapped("%s","New rows fade through green for nine seconds. Light-green age text marks the newest reception batch until a newer batch arrives.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Teal SNR means at least -10 dB; muted SNR means below -10 dB. This is a display threshold, not a calibrated reception-quality grade. SNR is measured by this receiver. Callsign and grid text remain neutral.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Map markers and paths",0)){
  ImGui::TextWrapped("%s","The house marks home, the origin for distance and bearing. Turquoise circles mark heard stations; light-green halos mark the latest reception batch. Nearby markers may group at the current zoom.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Color based on SNR starts off. When enabled, map circle fills vary continuously from red at -30 dB through yellow at -10 dB to green at +10 dB. A legend shows the range; tooltips show numeric SNR. Selected stations keep gold outlines and newest receptions keep separate halos. Dark-blue curved moving dots connect the transmitter to the station it addresses. General calls create no arc. One arc per station pair, frequency and mode; the latest reply determines its direction. Both endpoints need received grid evidence, which may be cached. An addressed exchange does not prove a completed contact.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Country shading uses your selected color and opacity, initially faint green. Night shading follows the solar day/night boundary. Map filters apply to station markers and exchange arcs.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Station hover shows the latest five distinct messages on that marker's band and mode. Each line starts with its latest UTC time, followed by the message in quotes. Repeats occupy one row.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Station actions",0)){
  ImGui::TextWrapped("%s","Right-click a station marker, main table row or details table row for the same callsign lookup, copying, favorite and ignore menu. Look up addressed station searches Google, QRZ or HamQTH for the target callsign. Ignore accepts unresolved or malformed transmitter identities and confirms before removing matching receptions from views, statistics and known saved logs. With no transmitter identity, it ignores only repetitions of that exact message. Other unidentified messages are kept. Unignore all allows future receptions; deleted records stay deleted.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Session and saved history",0)){
  ImGui::TextWrapped("%s","Both saved-history options start off and are saved when changed. Use saved history for stats includes older receptions only in the top statistics. Use saved history for everything also restores map markers, table rows and first-heard times. Filters still apply; time windows show All history. Calculations run in the background; uptime remains this session's active decoding time.");ImGui::Spacing();
  ImGui::TextWrapped("%s","By default, counters start fresh when Brown starts or the plugin is re-enabled. Saved-history options include retained reception records when enabled. Recent stations always counts only the last five minutes. Uptime counts active decoding time in this session.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Brown starts with a clear table and map unless Use saved history for everything is enabled. Clear keeps saved records. Reset stats turns off both saved-history options and starts a fresh session count. Clearing without resetting stats hides older entries only in the selected views. Turn saved history for everything off and on to restore them.");ImGui::Spacing();
 }
 ImGui::EndChild();ImGui::Separator();
 if(ImGui::Button("Close"))ImGui::CloseCurrentPopup();
 ImGui::EndPopup();
}
