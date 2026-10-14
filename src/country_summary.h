// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
namespace dx {
struct CountrySummary {std::string country;int64_t stations=0,distanceSamples=0,lastUtc=0;std::optional<double> averageKm;double averageSnr=0;std::vector<Decode> latestSenders;};
std::vector<CountrySummary> countrySummaries(const std::vector<Row>& rows,int sort=5,bool descending=true);
std::string senderList(const CountrySummary& summary);
}
