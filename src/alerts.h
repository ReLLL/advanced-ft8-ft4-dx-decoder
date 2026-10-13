// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include <set>
namespace dx {
inline constexpr const char* alertSoundNames[]={"None","Basso","Blow","Bottle","Frog","Funk","Glass","Hero","Morse","Ping","Pop","Purr","Sosumi","Submarine","Tink"};
inline std::string alertTone(const Json& sounds,const std::string& kind){
 const char* fallback=kind=="distance"?"Hero":kind=="country"?"Glass":kind=="grid"?"Tink":kind=="watch"?"Morse":"Pop";
 auto it=sounds.find(kind);auto tone=it!=sounds.end()&&it->is_string()?it->get<std::string>():std::string(fallback);
 for(auto name:alertSoundNames)if(tone==name)return tone;return fallback;
}
struct AlertPolicy {Filter filter;int64_t seconds=3600;bool distance=true,station=false,country=false,grid=false,watch=false,sound=false;std::string watchCalls;Json sounds=Json::object();};
struct Alert {std::string text;bool sound=false;int64_t utc=0;std::string tone;};
class Alerts {
public:
 void baseline(const std::vector<Row>& rows){farthest_=0;countries_.clear();grids_.clear();for(auto& r:rows){auto& d=r.decode;if(d.km&&d.gridSource=="received")farthest_=std::max(farthest_,*d.km);if(!d.country.empty())countries_.insert(d.country);if(d.gridSource=="received")grids_.insert(d.identity.grid);}}
 std::optional<Alert> evaluate(const std::vector<Decode>& batch,const AlertPolicy& policy,int64_t now){
  std::optional<Decode> candidate;std::string reason,kind;
  for(auto& d:batch){if(d.source!="live"||d.utc<watermark_||now-d.utc>60||d.utc>now+30||!policy.filter.accepts(d))continue;
   bool record=d.km&&d.gridSource=="received"&&*d.km>farthest_,newCountry=!d.country.empty()&&!countries_.count(d.country),newGrid=d.gridSource=="received"&&!grids_.count(d.identity.grid);
   if(record)farthest_=*d.km;if(!d.country.empty())countries_.insert(d.country);if(d.gridSource=="received")grids_.insert(d.identity.grid);
   bool watch=policy.watch&&!policy.watchCalls.empty()&&matches(d.identity.transmitter,policy.watchCalls);
   if((record&&policy.distance)||(d.firstCall&&policy.station)||(newCountry&&policy.country)||(newGrid&&policy.grid)||watch){if(!candidate||d.km.value_or(0)>candidate->km.value_or(0)){candidate=d;kind=record&&policy.distance?"distance":watch?"watch":newCountry&&policy.country?"country":d.firstCall&&policy.station?"station":"grid";reason=kind=="distance"?"New distance record":kind=="watch"?"Watch-list station heard":kind=="country"?"New country heard":kind=="station"?"New station heard":"New grid heard";}}
  }
  for(auto& d:batch)if(d.source=="live")watermark_=std::max(watermark_,d.utc+1);
  if(!candidate||now-lastAlert_<30)return {};lastAlert_=now;
  auto text=reason+": "+candidate->identity.transmitter;if(candidate->km)text+=" "+std::to_string(int(*candidate->km))+" km";auto tone=alertTone(policy.sounds,kind);return Alert{text,policy.sound&&tone!="None",candidate->utc,tone};
 }
 void suppressBefore(int64_t utc){watermark_=utc;}
private:
 double farthest_=0;std::set<std::string> countries_,grids_;int64_t watermark_=0,lastAlert_=0;
};
}
