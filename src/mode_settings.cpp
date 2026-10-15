// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
bool DXNotebook::modeEnabled(const std::string& name)const{
 if(name=="FT8")return ft8_;if(name=="FT4")return ft4_;auto spec=dx::modeSpec(name);return spec&&spec->available&&modeSettings_.value(name,dx::Json::object()).value("enabled",false);
}
std::vector<dx::Preset> DXNotebook::additionalPresets()const{
 std::vector<dx::Preset> result;
 // Dial frequencies from the pinned official WSJT-X FrequencyList.
 for(const char* mode:{"WSPR","FST4W"})for(int64_t dial:{136000,474200,1836600,3568600,7038600,10138700,14095600,18104600,21094600,24924600,28124600})result.push_back({dial==1836600&&std::string(mode)=="FST4W"?1836800:dial,mode});
 for(int64_t dial:{136000,474200,1839000})result.push_back({dial,"FST4"});
 for(int64_t dial:{50275000,144180000,432065000,1296065000})result.push_back({dial,"Q65"});return result;
}
bool DXNotebook::modeSettings(){
 bool changed=false;
 for(const auto& spec:dx::modeSpecs()){
  if(spec.name=="FT8"||spec.name=="FT4"||!spec.available)continue;std::string name(spec.name);auto config=modeSettings_.value(name,dx::Json::object());bool enabled=config.value("enabled",false);ImGui::PushID(name.c_str());
  if(ImGui::Checkbox(name.c_str(),&enabled)){config["enabled"]=enabled;changed=true;}
  if(enabled){
   int period=config.value("period_ms",name=="FST4"||name=="Q65"?60000:120000);if(!dx::capturePeriodMs(name,period))period=spec.periodsMs[0];auto label=std::to_string(period/1000)+" s";ImGui::SameLine();ImGui::SetNextItemWidth(95);
   if(ImGui::BeginCombo("Sequence",label.c_str())){for(size_t i=0;i<spec.periodCount;i++){int value=spec.periodsMs[i];auto text=std::to_string(value/1000)+" s";if(ImGui::Selectable(text.c_str(),value==period)){period=value;config["period_ms"]=period;changed=true;}}ImGui::EndCombo();}
   if(name=="Q65"){int maximum=period==15000?2:period==30000?3:4;int submode=std::clamp(config.value("submode",0),0,maximum);const char* names[]={"A","B","C","D","E"};ImGui::SameLine();ImGui::SetNextItemWidth(65);if(ImGui::Combo("Spacing",&submode,names,maximum+1))changed=true;config["submode"]=submode;}
   config["period_ms"]=period;
  }
  modeSettings_[name]=std::move(config);ImGui::PopID();
 }
 ImGui::TextWrapped("New modes start off. Match the sequence length and Q65 spacing used on air. Reception starts at the next complete UTC sequence. Tuning or stopping discards partial audio. Q65 uses individual sequences without cross-sequence averaging.");
 return changed;
}
