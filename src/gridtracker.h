// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include <deque>
#include <map>
#include <set>
namespace dx {
namespace wsjt {
using Packet=std::vector<uint8_t>;
std::string identity(const Preset& preset);
Packet header(uint32_t type,const std::string& id);
Packet heartbeat(const std::string& id);
Packet status(const Preset& preset,const std::string& call,const std::string& grid,bool receiving);
Packet decode(const Decode& row,bool fresh);
std::optional<std::pair<uint32_t,std::string>> request(const uint8_t* bytes,size_t count);
}
struct GridTrackerContext {std::vector<Preset> channels;std::string call="SWL",grid;};
struct GridTrackerState {bool enabled=false;uint64_t sent=0;int64_t lastReply=0;std::string error;};
// Worker-thread only. Numeric IPs avoid DNS stalls; socket IO is nonblocking.
class GridTracker {
public:
 ~GridTracker(){close();}
 void tick(Store& store,const Json& settings,const GridTrackerContext& context,bool receiving,int64_t utc,int64_t milliseconds);
 const GridTrackerState& state()const{return state_;}
private:
 bool send(const wsjt::Packet& packet);void close();
 void remember(const Preset& preset);void replay(Store& store,int64_t utc);
 intptr_t socket_=-1;std::string address_;int port_=0,minutes_=30;bool replay_=true;
 int64_t cursor_=0,lastPulse_=0,lastReplay_=-10000,retryAt_=0;
 std::map<std::string,Preset> sessions_;std::set<std::string> announced_;
 std::deque<std::pair<Decode,bool>> pending_;GridTrackerState state_;
};
}
