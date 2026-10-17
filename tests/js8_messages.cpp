// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "js8_messages.h"
#include <varicode.h>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
JS8::Event::Decoded frame(QString data,int bits,int mode=0,int frequency=1500){return {120000,-15,.1f,float(frequency),data.toStdString(),bits,1.f,mode};}
int main(){
 dx::JS8Messages messages;int n=0;auto heartbeat=Varicode::packHeartbeatMessage("HB FN42","K1ABC",&n);CHECK(!heartbeat.isEmpty());
 auto result=messages.accept(frame(heartbeat,3),1800000000,7078000);CHECK(result.size()==1&&result[0]["message_complete"]==true);auto identity=dx::receptionMeaning("JS8",result[0]["message"]);CHECK(identity.transmitter=="K1ABC"&&identity.grid=="FN42"&&identity.type=="Heartbeat");
 auto cq=Varicode::packHeartbeatMessage("CQ CQ CQ FN42","K1ABC",&n);CHECK(!cq.isEmpty());result=messages.accept(frame(cq,3),1800000015,7078000);CHECK(dx::receptionMeaning("JS8",result.at(0).at("message")).called=="CQ");
 auto run=[&](const QString& sender,const QString& text,int64_t utc,int skipped=-1){auto frames=Varicode::buildMessageFrames(sender,"FN42",{},text,true,false,0);dx::Json rows=dx::Json::array();for(int i=0;i<frames.size();i++){if(i==skipped)continue;auto event=frame(frames[i].first,frames[i].second);for(auto& row:messages.accept(event,utc+i*15,7078000))rows.push_back(row);}return rows;};
 result=run("K1ABC","W9XYZ HELLO THIS IS A LONG MESSAGE FROM MY STATION TESTING JS8 RECEPTION",1800000100);
 CHECK(!result.empty()&&result.back()["message_complete"]==true);std::string text=result.back()["message"];std::cerr<<text<<'\n';CHECK(text.find("HELLO THIS IS A LONG MESSAGE FROM MY STATION TESTING JS8 RECEPTION")!=std::string::npos);identity=dx::receptionMeaning("JS8",text);CHECK(identity.transmitter=="K1ABC"&&identity.called=="W9XYZ"&&identity.grid.empty());CHECK(dx::Json::parse(result.back()["raw"].get<std::string>()).size()>1);
 result=run("K1ABC/P","W9XYZ HELLO THIS IS A PORTABLE STATION",1800000400);CHECK(!result.empty()&&result.back()["message_complete"]==true);CHECK(dx::receptionMeaning("JS8",result.back()["message"]).transmitter=="K1ABC/P");
 result=run("K1ABC","W9XYZ MSG THIS MESSAGE HAS A REQUIRED CHECKSUM",1800000500);CHECK(!result.empty()&&result.back()["message_complete"]==true);CHECK(result.back()["message"]=="K1ABC: W9XYZ MSG THIS MESSAGE HAS A REQUIRED CHECKSUM");
 auto corrupt=Varicode::buildMessageFrames("K1ABC","FN42",{},"W9XYZ MSG THIS MESSAGE HAS A REQUIRED CHECKSUM",true,false,0);CHECK(corrupt.size()>1);corrupt.back().first=Varicode::packDataMessage("BAD CHECKSUM",&n);result=dx::Json::array();for(int i=0;i<corrupt.size();i++)for(auto& row:messages.accept(frame(corrupt[i].first,corrupt[i].second),1800000600+i*15,7078000))result.push_back(row);CHECK(!result.empty()&&result.back()["message_complete"]==false);
 result=run("K1ABC","W9XYZ HELLO THIS IS A LONG MESSAGE FROM MY STATION TESTING MISSING FRAMES",1800000700,1);CHECK(!result.empty()&&result.back()["message_complete"]==false);CHECK(dx::receptionMeaning("JS8",result.back()["message"]).transmitter.empty());
 auto data=Varicode::packDataMessage("HELLO WORLD",&n);CHECK(!data.isEmpty());result=messages.accept(frame(data,2),1800001000,7078000);CHECK(!result.empty()&&!result[0]["message_complete"].get<bool>()&&dx::receptionMeaning("JS8",result[0]["message"]).transmitter.empty());
 result=messages.accept(frame(heartbeat,3),1800001015,7078000,true);CHECK(dx::receptionMeaning("JS8",result[0]["message"]).transmitter.empty());
 auto weak=frame(heartbeat,3);weak.quality=.1f;CHECK(messages.accept(weak,1800001020,7078000).empty());
 // Only explicit heartbeat/CQ grids count. A grid mentioned in prose is not a location.
 CHECK(dx::receptionMeaning("JS8","K1ABC: W9XYZ HELLO QF56").grid.empty());CHECK(dx::receptionMeaning("JS8","[JS8 fragment] K1ABC: W9XYZ HELLO").transmitter.empty());CHECK(dx::receptionMeaning("JS8","@ALLCALL: HELLO").transmitter.empty());
 CHECK(dx::messageKind(dx::receptionMeaning("JS8","K1ABC: @HB HEARTBEAT FN42"))==dx::MessageKind::Beacon);CHECK(dx::messageKind(dx::receptionMeaning("JS8","K1ABC: @JS8 HELLO"))==dx::MessageKind::Other);CHECK(dx::messageKind(dx::receptionMeaning("JS8","K1ABC: W9XYZ HELLO"))==dx::MessageKind::Exchange);
 std::cout<<"Upstream JS8 framing, heartbeat/CQ, multi-frame and portable identities, missing frames, ambiguity and weak-decode rejection passed\n";
}
