// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"

void DXNotebook::help(){
 ImGui::SetNextWindowSize({720,620},ImGuiCond_FirstUseEver);
 if(!ImGui::BeginPopupModal("Decoder help",nullptr,ImGuiWindowFlags_None))return;
 popup_=true;
 ImGui::BeginChild("help_content",{0,-ImGui::GetFrameHeightWithSpacing()-8},false);
 if(ImGui::CollapsingHeader("First-heard times",ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s","Msg age shows how long ago the latest reception occurred. A repeated message refreshes that value.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Msg first heard is the first reception of the exact message from the same callsign in this session. Repeats keep that timestamp. A different message has its own first-heard time.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Station first heard is the first reception of any message from that callsign in this session, across all bands and modes.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Both first-heard columns show elapsed seconds. Hover shows the original UTC date and time. Brown restart starts both again with new receptions. Click a header to sort by its timestamp.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Distances and bearing",ImGuiTreeNodeFlags_DefaultOpen)){
  ImGui::TextWrapped("%s","Distances are approximate great-circle distances from Brown's home Maidenhead grid center to the station's received grid center. A four-character grid covers an area, so its center is not an exact transmitter location.");ImGui::Spacing();
  ImGui::TextWrapped("%s","An asterisk marks a cached grid received within the preceding six hours. Repeated messages without a grid do not renew the evidence. No recent received grid means unknown distance. Callsign prefixes identify a DXCC entity, not a precise station location.");ImGui::Spacing();
  ImGui::TextWrapped("%s","The legacy decoder can report a different distance because it uses a country or regional reference point. Our distances use received grid evidence.");ImGui::Spacing();
  ImGui::TextWrapped("%s","DX activity is the circular mean of geographic short-path bearings to the farthest recent unique stations, measured from home. It describes heard activity, not measured arrival direction or a propagation forecast. Mixed directions means the bearings are too dispersed for one useful direction.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Table colors",0)){
  ImGui::TextWrapped("%s","Green country: the first reception from that country in this session. Gold bold country and distance: the farthest station in the selected statistics scope. Gold takes precedence over green.");ImGui::Spacing();
  ImGui::TextWrapped("%s","New rows fade through green for nine seconds. Light-green age text marks the newest reception batch until a newer batch arrives.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Teal SNR means at least -10 dB; muted SNR means below -10 dB. This is a display threshold, not a calibrated reception-quality grade. SNR is measured by this receiver. Callsign and grid text remain neutral.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Map markers and paths",0)){
  ImGui::TextWrapped("%s","The house marks home, the origin for distance and bearing. Turquoise circles mark heard stations; light-green halos mark the latest reception batch. Nearby markers may group at the current zoom.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Dark-blue curved moving dots connect the transmitter to the station it addresses. General calls create no arc. One arc per station pair, frequency and mode; the latest reply determines its direction. Both endpoints need received grid evidence, which may be cached. An addressed exchange does not prove a completed contact.");ImGui::Spacing();
  ImGui::TextWrapped("%s","A gold line from home to a selected station is a distance guide. It is separate from communication arcs between stations.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Country shading uses your selected color and opacity, initially faint green. Night shading follows the solar day/night boundary. Map filters apply to station markers and exchange arcs.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Station hover shows the latest five distinct messages on that marker's band and mode. Each line starts with its latest UTC time, followed by the message in quotes. Repeats occupy one row.");ImGui::Spacing();
 }
 if(ImGui::CollapsingHeader("Session and saved history",0)){
  ImGui::TextWrapped("%s","Use saved history for stats starts off. Enable it to include older saved receptions in the top statistics. Calculations run in the background; uptime remains this session's active decoding time.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Countries, unique stations, farthest reception, recent stations and uptime start fresh when Brown starts or the plugin is re-enabled. Uptime counts active decoding time.");ImGui::Spacing();
  ImGui::TextWrapped("%s","Brown starts with a clear table and map. The first-heard times and session counters start fresh. Saved reception history and the persistent CSV are retained for logging and history access.");ImGui::Spacing();
 }
 ImGui::EndChild();ImGui::Separator();
 if(ImGui::Button("Close"))ImGui::CloseCurrentPopup();
 ImGui::EndPopup();
}
