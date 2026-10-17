// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
namespace dx {
std::string Store::distanceSql(bool payload)const{
 auto field=[&](const char* name){return payload?"json_extract(payload,'$."+std::string(name)+"')":std::string(name);};
 auto km=field("km");return countryDistances_?"coalesce("+km+",CASE WHEN call!='' AND call NOT LIKE '<%' AND "+field("grid")+"='' THEN dx_country_distance("+field("country")+") END)":km;
}
std::optional<LocationCheck::CountryDistance> Store::countryDistance(const std::string& country){return countryDistances_?locationCheck_.nearestCountryGrid(country,distanceHome_):std::nullopt;}
void Store::projectDistance(Decode& d){
 if(!d.countryDistanceGrid.empty()){d.countryDistanceGrid.clear();d.km.reset();}
 if(d.km||d.position||!d.identity.grid.empty()||d.identity.transmitter.empty()||d.identity.transmitter.front()=='<')return;
 if(auto estimate=countryDistance(d.country)){d.km=estimate->km;d.countryDistanceGrid=estimate->grid;}
}
}
