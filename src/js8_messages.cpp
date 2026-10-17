// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "js8_messages.h"
#include <varicode.h>
#include <cmath>
namespace dx {
namespace {
bool validCall(const std::string& call){bool compound=false;return !call.empty()&&call.front()!='<'&&call.front()!='@'&&Varicode::isValidCallsign(QString::fromStdString(call),&compound);}
std::string clean(QString text){return text.trimmed().toStdString();}
}
void JS8Messages::expire(int64_t utc){for(auto it=streams_.begin();it!=streams_.end();)if(utc-it->second.utc>90)it=streams_.erase(it);else ++it;}
Json JS8Messages::accept(const JS8::Event::Decoded& event,int64_t utc,int64_t dial,bool ambiguous){
 Json out=Json::array();expire(utc);if(event.data.size()!=12||event.quality<.17f||!std::isfinite(event.frequency)||!std::isfinite(event.xdt)||event.frequency<0||event.frequency>6000)return out;
 DecodedText decoded(event);if(decoded.frameType()==Varicode::FrameUnknown)return out;
 const bool first=event.type&Varicode::JS8CallFirst,last=event.type&Varicode::JS8CallLast;
 auto raw=Json{{"timestamp",utc},{"frame",event.data},{"bits",event.type},{"snr",event.snr},{"dt",event.xdt},{"audio_hz",event.frequency},{"submode",event.mode}};
 auto record=[&](const std::string& text,bool complete,const Json& frames){out.push_back({{"timestamp",utc},{"message",text},{"message_complete",complete},{"raw",frames.dump()},{"snr",event.snr},{"dt",event.xdt},{"audio_hz",int(std::lround(event.frequency))},{"submode",event.mode}});};
 // Only a unique, uninterrupted stream at this offset may provide identity.
 std::vector<int> matches;for(const auto& [id,s]:streams_)if(s.dial==dial&&s.submode==event.mode&&std::abs(s.audio-event.frequency)<=10)matches.push_back(id);
 int id=matches.size()==1?matches.front():-1;
 if(ambiguous){for(int match:matches)streams_.erase(match);record("[JS8 ambiguous fragment] "+clean(decoded.message()),false,Json::array({raw}));return out;}
 if(decoded.isHeartbeat()){
  if(!validCall(clean(decoded.compoundCall())))return out;
  for(int match:matches)streams_.erase(match);
  record(clean(decoded.message()),true,Json::array({raw}));return out;
 }
 auto directed=decoded.directedMessage();std::string sender=directed.isEmpty()?"":clean(directed[0]);
 const bool compound=decoded.frameType()==Varicode::FrameCompound;
 const bool header=compound||decoded.isDirectedMessage();
 if(header){
  if(compound)sender=clean(decoded.compoundCall());
  if(!validCall(sender)){
   // A compound sender is sent in a separate, immediately preceding frame.
   if(sender!="<....>"||id<0||!streams_[id].compound||utc-streams_[id].utc>60){record(clean(decoded.message())+" [incomplete]",false,Json::array({raw}));return out;}
   sender=streams_[id].sender;
  }else{
   for(int match:matches)streams_.erase(match);
   if(streams_.size()>=64)streams_.erase(streams_.begin());id=++serial_;streams_[id]={};
  }
  auto& stream=streams_[id];stream.sender=sender;stream.dial=dial;stream.submode=event.mode;stream.audio=int(std::lround(event.frequency));stream.utc=utc;stream.compound=compound;
  if(compound){stream.prefix=sender+": ";stream.body.clear();}
  else{stream.target=clean(directed.value(1));stream.command=directed.value(2).toStdString();stream.prefix=sender+": "+stream.target+stream.command;if(directed.size()>3)stream.prefix+=" "+clean(directed[3]);stream.prefix+=" ";stream.body.clear();}
 }else if(decoded.frameType()==Varicode::FrameData){
  // A new first frame must never inherit an older sender. Ambiguous offsets
  // and missing headers are kept as unattributed fragments.
  if(first||id<0){for(int match:matches)streams_.erase(match);record("[JS8 fragment] "+clean(decoded.message()),false,Json::array({raw}));return out;}
  auto& stream=streams_[id];int period=event.mode==4?30:event.mode==0?15:event.mode==1?10:6;
  if(utc-stream.utc!=period){streams_.erase(id);record("[JS8 fragment] "+clean(decoded.message()),false,Json::array({raw}));return out;}
  stream.body+=decoded.message().toStdString();stream.utc=utc;
 }else return out;
 auto& stream=streams_[id];stream.frames.push_back(raw);stream.audio=int(std::lround(event.frequency));
 if(stream.body.size()>4096||stream.frames.size()>64){streams_.erase(id);return out;}
 bool complete=last&&(!compound||decoded.frameType()==Varicode::FrameData);auto body=QString::fromStdString(stream.body);bool checksumValid=true;
 if(complete&&!stream.command.empty()&&Varicode::isCommandBuffered(QString::fromStdString(stream.command))){
  body=Varicode::rstrip(body);auto size=Varicode::isCommandChecksumed(QString::fromStdString(stream.command));
  if(size){body=Varicode::lstrip(body);int width=size==32?6:3;auto checksum=body.right(width);auto text=body.left(std::max(0,int(body.size())-width-1));checksumValid=body.size()>width&&(size==32?Varicode::checksum32Valid(checksum,text):Varicode::checksum16Valid(checksum,text));if(checksumValid)body=text;}
 }
 complete=complete&&checksumValid;auto text=QString::fromStdString(stream.prefix)+body;
 record(clean(text)+(complete?"":" [incomplete]"),complete,stream.frames);
 if(last)streams_.erase(id);return out;
}
}
