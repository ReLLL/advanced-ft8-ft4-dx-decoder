// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "country_summary.h"
#include <map>
#include <algorithm>
namespace dx {
std::string senderList(const CountrySummary& summary){std::string out;for(const auto& d:summary.latestSenders){if(!out.empty())out+=", ";out+=d.identity.transmitter;}return out;}
std::vector<CountrySummary> countrySummaries(const std::vector<Row>& rows,int sort,bool descending){
 std::map<std::string,Decode> senders;for(const auto& row:rows){const auto& d=row.decode;const auto& call=d.identity.transmitter;if(call.empty()||call[0]=='<')continue;auto found=senders.find(call);if(found==senders.end()||d.utc>found->second.utc)senders[call]=d;}
 std::map<std::string,CountrySummary> countries;for(const auto& entry:senders){const auto& d=entry.second;if(d.country.empty())continue;auto& country=countries[d.country];country.country=d.country;country.stations++;country.averageSnr+=d.snr;if(d.km){country.averageKm=country.averageKm.value_or(0)+*d.km;country.distanceSamples++;}country.lastUtc=std::max(country.lastUtc,d.utc);country.latestSenders.push_back(d);}
 std::vector<CountrySummary> out;for(auto& entry:countries){auto& c=entry.second;c.averageSnr/=double(c.stations);if(c.averageKm)c.averageKm=*c.averageKm/double(c.distanceSamples);std::sort(c.latestSenders.begin(),c.latestSenders.end(),[](const Decode& a,const Decode& b){return a.utc!=b.utc?a.utc>b.utc:a.identity.transmitter<b.identity.transmitter;});if(c.latestSenders.size()>5)c.latestSenders.resize(5);out.push_back(std::move(c));}
 std::sort(out.begin(),out.end(),[&](const CountrySummary& a,const CountrySummary& b){int cmp=0;if(sort==2&&a.averageKm.has_value()!=b.averageKm.has_value())return a.averageKm.has_value();auto compare=[](const auto& x,const auto& y){return x<y?-1:x>y?1:0;};switch(sort){case 0:cmp=compare(a.country,b.country);break;case 1:cmp=compare(a.stations,b.stations);break;case 2:cmp=compare(a.averageKm.value_or(0),b.averageKm.value_or(0));break;case 3:cmp=compare(a.averageSnr,b.averageSnr);break;case 4:cmp=compare(senderList(a),senderList(b));break;default:cmp=compare(a.lastUtc,b.lastUtc);}return cmp?(descending?cmp>0:cmp<0):a.country<b.country;});return out;
}
}
