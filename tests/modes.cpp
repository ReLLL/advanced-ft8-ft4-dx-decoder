// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "modes.h"
#include "engine.h"
#include <cmath>
#include <iostream>
#include <set>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 CHECK(dx::capturePeriodMs("FT8")==15000&&dx::capturePeriodMs("FT4")==7500);
 CHECK(dx::capturePeriodMs("WSPR")==120000&&dx::capturePeriodMs("FST4W",1800000)==1800000);
 CHECK(!dx::capturePeriodMs("FT4",15000)&&!dx::capturePeriodMs("FST4W",60000)&&!dx::capturePeriodMs("unknown"));
 std::set<dx::MarkerShape> extra;
 for(const auto& mode:dx::modeSpecs()){
  CHECK(mode.periodCount>0&&mode.periodCount<=mode.periodsMs.size());
  for(size_t i=0;i<mode.periodCount;i++)CHECK(dx::capturePeriodMs(mode.name,mode.periodsMs[i])==mode.periodsMs[i]);
  if(mode.name=="FT8"||mode.name=="FT4")CHECK(mode.available&&mode.marker==dx::MarkerShape::Circle&&!mode.beacon);
  else{CHECK(mode.available==bool(mode.name=="JS8"?DX_JS8:DX_ADDITIONAL_MODES)&&mode.marker!=dx::MarkerShape::Circle);CHECK(extra.insert(mode.marker).second);}
 }
 for(auto shape:extra){auto outline=dx::markerOutline(shape,6);CHECK(outline.count>=3&&outline.count<=8);double area=0;
  for(int i=0;i<outline.count;i++){auto a=outline.points[i],b=outline.points[(i+1)%outline.count];CHECK(std::abs(std::hypot(a[0],a[1])-6)<1e-5);area+=a[0]*b[1]-a[1]*b[0];}CHECK(area>0);
 }
 CHECK(dx::markerOutline(dx::MarkerShape::Circle,6).count==0&&dx::markerOutline(dx::MarkerShape::Square,NAN).count==0);
 CHECK(dx::modeMarker("unknown")==dx::MarkerShape::Circle);
 auto beacon=dx::receptionMeaning("WSPR","K1ABC FN42 37");CHECK(beacon.transmitter=="K1ABC"&&beacon.called.empty()&&beacon.grid=="FN42"&&dx::messageKind(beacon)==dx::MessageKind::Beacon);
 CHECK(dx::receptionMeaning("FST4W","<1234567> FN42 37").transmitter=="<1234567>");
 auto portable=dx::receptionMeaning("WSPR","PJ4/K1ABC 30");CHECK(portable.transmitter=="PJ4/K1ABC"&&portable.grid.empty()&&portable.called.empty());
 dx::Decode received;received.mode="FST4W";received.message="K1ABC FN42 37";received.snr=-16;received.periodMs=120000;auto restored=dx::Decode::fromJson(received.json());CHECK(restored.txPowerDbm==37&&restored.snr==-16&&restored.periodMs==120000);
 dx::SlotCapture capture;std::vector<float> second(12000,.1f);std::optional<dx::Job> slot;
 for(int64_t i=1;i<=120;i++)slot=capture.feed(second.data(),second.size(),i*1000,14095600,"WSPR");CHECK(slot&&slot->pcm.size()==1440000&&slot->periodMs==120000&&slot->utcMs==0);
 capture.reset();for(int64_t i=1;i<=14;i++)CHECK(!capture.feed(second.data(),second.size(),i*1000,7074000,"FT8"));CHECK(!capture.feed(second.data(),second.size(),15000,14074000,"FT8"));
 CHECK(!capture.feed(second.data(),second.size(),16000,14074000,"FST4W",60000));
 std::cout<<"Existing modes, protocol periods, capability gates and distinct marker geometry passed\n";return 0;
}
