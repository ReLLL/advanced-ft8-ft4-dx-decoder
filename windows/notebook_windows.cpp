// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "sharp_host.h"
#include <core.h>
#include <fstream>
#include <iostream>
#include <cmath>
void core::Settings::load(){
 std::ifstream input(dx::sharpHost().root/"receiver.json");auto value=dx::Json::parse(input,nullptr,false);if(value.is_object()&&value.value("moduleInstances",dx::Json{}).is_object())conf=std::move(value);
}
void core::Settings::release(bool save){
 auto copy=conf;mutex.unlock();if(!save)return;
 auto path=dx::sharpHost().root/"receiver.json",temporary=dx::sharpHost().root/"receiver.pending.json";
 std::ofstream file(temporary,std::ios::binary|std::ios::trunc);file<<copy.dump(2);file.close();
 std::error_code error;if(file)std::filesystem::rename(temporary,path,error);
 if(!file||error)std::cerr<<"Could not save receiver settings. Check the data folder permissions.\n";
}
void DXNotebook::showWindows(){openWindow();}
void DXNotebook::renderWindowsFrame(){draw();}
void DXNotebook::metadata(){
 std::lock_guard<std::mutex> lock(captureMutex_);
 auto& host=dx::sharpHost();
 if(auto point=dx::gridPoint(receiverGrid_);point&&(point->lat!=home_.lat||point->lon!=home_.lon)){home_=*point;engine_->home(home_);}
 auto center=host.center.load(),rate=host.rate.load();bool context=center!=center_||rate!=rate_;if(context){channels_.clear();engine_->invalidateAdditional();center_=center;rate_=rate;}
 auto configured=presets_;for(auto preset:additionalPresets())if(std::none_of(configured.begin(),configured.end(),[&](const auto& p){return p.dial==preset.dial&&p.mode==preset.mode;}))configured.push_back(preset);
 auto desired=dx::eligible(center,rate,configured);size_t modeBytes=0;int modeCount=0,js8Count=0;
 desired.erase(std::remove_if(desired.begin(),desired.end(),[&](auto& p){
  if(!modeEnabled(p.mode))return true;if(p.mode=="FT8"||p.mode=="FT4")return false;
  auto setting=modeSettings_.value(p.mode,dx::Json::object());p.periodMs=setting.value("period_ms",0);auto periods=dx::sequencePeriods(p.mode,p.periodMs);if(periods.empty())return true;
  p.submode=p.mode=="Q65"?std::clamp(setting.value("submode",0),0,dx::sequenceSubmodeMaximum(p.periodMs)):0;
  if(p.mode=="JS8")return js8Count++>=2;auto bytes=size_t(periods.back())*24+24000;if(modeCount>=4||modeBytes+bytes>96*1024*1024)return true;modeBytes+=bytes;++modeCount;return false;
 }),desired.end());if(desired.size()>8)desired.resize(8);
 bool same=channels_.size()==desired.size();if(same)for(size_t n=0;n<desired.size();n++){const auto& p=channels_[n]->preset;const auto& q=desired[n];same&=p.dial==q.dial&&p.mode==q.mode&&p.periodMs==q.periodMs&&p.submode==q.submode;}
 if(!same){channels_.clear();engine_->invalidateAdditional();for(auto p:desired)channels_.push_back(std::make_unique<DXChannel>(this,p,center));}
 auto playing=host.playing.load();bool ready=enabled_&&!paused_&&playing;
 dx::Json plan={{"kind","plan"},{"enabled",ready},{"presets",dx::Json::array()}};dx::GridTrackerContext forwarding;forwarding.grid=receiverGrid_;forwarding.call=receiverCall_;
 std::string bands,modes;for(auto& channel:channels_){
  if(channel->accept!=ready){channel->reset();engine_->invalidateAdditional();}channel->accept=ready;channel->replay=host.replay.load();auto& p=channel->preset;
  plan["presets"].push_back({{"dial",p.dial},{"mode",p.mode},{"period_ms",p.periodMs},{"submode",p.submode}});forwarding.channels.push_back(p);
  if(!bands.empty()){bands+=',';modes+=',';}bands+=dx::bandName(p.dial);modes+=p.mode;
 }
 plan["corner_rows"]=dx::Json::array();auto view=engine_->snapshot();plan["stations"]=view->stats.stations;plan["countries"]=view->stats.countries;
 auto cornerRow=[&](const dx::Decode& d,bool record,const std::string& key){auto flag=flagCodes_.find(dx::countryOutlineKey(d.country));return dx::Json{{"key",key},{"country",d.country.empty()?"Unknown":d.country},{"call",d.identity.transmitter.empty()?"--":d.identity.transmitter},{"distance",d.km?dx::distancePrefix(d)+dx::distanceSummary(*d.km,distanceMiles_):std::string("-- ")+dx::distanceUnit(distanceMiles_)},{"snr",std::string(d.snr>=0?"+":"")+std::to_string(d.snr)+" dB"},{"message",d.message},{"flag",flag==flagCodes_.end()?"":flag->second},{"record",record}};};
 if(enabled_&&cornerEnabled_&&settingsLoaded_){auto now=dx::monotonicMs();for(auto i=view->cornerReceptions.rbegin();i!=view->cornerReceptions.rend()&&plan["corner_rows"].size()<75;++i)if(now-i->observedMs<int64_t(cornerSeconds_)*1000)plan["corner_rows"].push_back(cornerRow(i->decode,false,std::to_string(i->id)));if(view->stats.farthestReception)plan["corner_rows"].push_back(cornerRow(*view->stats.farthestReception,true,view->stats.farthestReception->key()));}
 host.plan(std::move(plan));engine_->active(ready&&!channels_.empty());engine_->alertContext(bands,ft8_,ft4_,modes);engine_->gridTrackerContext(std::move(forwarding));
}
void DXNotebook::receiveWindowsAudio(dx::SharpAudio block){
 std::lock_guard<std::mutex> lock(captureMutex_);
 if(block.header.value("kind","")=="reset"){for(auto& channel:channels_)channel->reset();engine_->invalidateAdditional();return;}
  for(auto& channel:channels_){const auto& p=channel->preset;
   if(block.header.value("dial",int64_t(0))==p.dial&&block.header.value("mode","")==p.mode&&block.header.value("period_ms",0)==p.periodMs&&block.header.value("submode",0)==p.submode)
    channel->feed(block.iq,block.header.value("sample_rate",0.),block.header.value("end_ms",int64_t(0)),block.header.value("sequence",uint64_t(0)));
  }
}
