// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
bool DXNotebook::modeEnabled(const std::string& name)const{
 if(name=="FT8")return ft8_;if(name=="FT4")return ft4_;auto spec=dx::modeSpec(name);return spec&&spec->available&&modeSettings_.value(name,dx::Json::object()).value("enabled",name=="WSPR");
}
std::vector<dx::Preset> DXNotebook::additionalPresets()const{
 std::vector<dx::Preset> result;
 // Dial frequencies from the pinned official WSJT-X FrequencyList.
 for(const char* mode:{"WSPR","FST4W"})for(int64_t dial:{136000,474200,1836600,3568600,7038600,10138700,14095600,18104600,21094600,24924600,28124600})result.push_back({dial==1836600&&std::string(mode)=="FST4W"?1836800:dial,mode});
 for(int64_t dial:{136000,474200,1839000})result.push_back({dial,"FST4"});
 for(int64_t dial:{50275000,144180000,432065000,1296065000})result.push_back({dial,"Q65"});
 for(int64_t dial:{1842000,3578000,7078000,10130000,14078000,18104000,21078000,24922000,28078000,50318000,144178000})result.push_back({dial,"JS8"});return result;
}
bool DXNotebook::modeSettings(){
 bool changed=false;
 for(const auto& spec:dx::modeSpecs()){
  if(spec.name=="FT8"||spec.name=="FT4"||!spec.available)continue;std::string name(spec.name);auto config=modeSettings_.value(name,dx::Json::object());bool enabled=config.value("enabled",name=="WSPR");ImGui::PushID(name.c_str());
  if(ImGui::Checkbox(name.c_str(),&enabled)){config["enabled"]=enabled;changed=true;}
  if(name=="JS8"&&ImGui::IsItemHovered())ImGui::SetTooltip("Receive JS8Call messages using a separate decoder. Starts off.\nAll checks Normal, Fast, Slow and Turbo / JS8 40.\nMissing frames are marked incomplete; unknown senders stay unknown.\nExperimental JS8 60 is excluded.");
  if(enabled){
   auto periodLabel=[&](int value){return name=="JS8"?std::string(value==6000?"Turbo (6 s)":value==10000?"Fast (10 s)":value==15000?"Normal (15 s)":"Slow (30 s)"):std::to_string(value/1000)+" s";};
   int period=config.value("period_ms",0);if(dx::sequencePeriods(name,period).empty())period=0;auto label=period?periodLabel(period):"All";ImGui::SameLine();ImGui::SetNextItemWidth(name=="JS8"?145:95);
   if(ImGui::BeginCombo(name=="JS8"?"Speed":"Sequence",label.c_str())){if(ImGui::Selectable("All",period==0)){period=0;changed=true;}for(size_t i=0;i<spec.periodCount;i++){int value=spec.periodsMs[i];auto text=periodLabel(value);if(ImGui::Selectable(text.c_str(),value==period)){period=value;changed=true;}}ImGui::EndCombo();}
   if(name=="Q65"){int maximum=dx::sequenceSubmodeMaximum(period);int submode=std::clamp(config.value("submode",0),0,maximum);const char* names[]={"A","B","C","D","E"};ImGui::SameLine();ImGui::SetNextItemWidth(65);if(ImGui::Combo("Spacing",&submode,names,maximum+1))changed=true;config["submode"]=submode;if(period==0&&ImGui::IsItemHovered())ImGui::SetTooltip("All sequence lengths share spacing A, B or C.\nChoose 30 s or longer for spacing D; 60 s or longer for E.");}
   config["period_ms"]=period;
  }
  modeSettings_[name]=std::move(config);ImGui::PopID();
 }
 ImGui::TextWrapped("FT8, FT4 and WSPR start on; other modes start off. All sequence lengths are selected by default and share captured audio. Longer sequences need a full uninterrupted capture, up to 30 minutes. Choose one length to reduce decoder work. Match Q65 spacing; All supports A-C. Tuning or stopping discards partial audio. Q65 uses individual sequences without cross-sequence averaging.");
 return changed;
}
