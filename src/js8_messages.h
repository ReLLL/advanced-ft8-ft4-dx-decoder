// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include <JS8.hpp>
#include <decodedtext.h>
#include <map>
namespace dx {
// Lives only in the isolated JS8 process. Resetting it cannot alter saved history.
class JS8Messages {
public:
 Json accept(const JS8::Event::Decoded&,int64_t utc,int64_t dial,bool ambiguous=false);
 void expire(int64_t utc);
private:
 struct Stream {int64_t utc=0,dial=0;int submode=0,audio=0;std::string sender,target,command,prefix,body;Json frames=Json::array();bool compound=false;};
 std::map<int,Stream> streams_;int serial_=0;
};
}
