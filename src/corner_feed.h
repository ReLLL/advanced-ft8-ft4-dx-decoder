// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include <algorithm>
namespace dx {
struct CornerReception {int64_t id=0,observedMs=0;Decode decode;};
// Monotonic arrival times keep clock corrections and saved history out of the feed.
class CornerFeed {
public:
 void add(int64_t id,const Decode& decode,int64_t observedMs){rows_.push_back({id,observedMs,decode});}
 void expire(int64_t nowMs,int seconds){auto lifetime=int64_t(std::clamp(seconds,2,120))*1000;rows_.erase(std::remove_if(rows_.begin(),rows_.end(),[&](const auto& row){return nowMs-row.observedMs>=lifetime;}),rows_.end());}
 void remove(const IgnoreTarget& target){rows_.erase(std::remove_if(rows_.begin(),rows_.end(),[&](const auto& row){return target.matches(row.decode);}),rows_.end());}
 void clear(){rows_.clear();}
 const std::vector<CornerReception>& rows()const{return rows_;}
private:
 std::vector<CornerReception> rows_;
};
}
