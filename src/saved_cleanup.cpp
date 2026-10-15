// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "saved_cleanup.h"
#include <ctime>
#include <cmath>
#include <stdexcept>
namespace dx {
std::optional<int64_t> cleanupDate(const std::string& text,bool endOfDay){
 return localDateTimeParse(text,endOfDay);
}
bool SavedCleanup::valid()const{auto k=int(kind);return k>=0&&k<=6&&((k<=2&&from>0&&(kind!=CleanupKind::Between||to>=from))||(k==3||k==4?!country.empty():k>=5));}
bool SavedCleanup::matches(const Decode& d)const{
 if(!valid())return false;
 switch(kind){
  case CleanupKind::Before:return d.utc<from;
  case CleanupKind::After:return d.utc>from;
  case CleanupKind::Between:return d.utc>=from&&d.utc<=to;
  case CleanupKind::Country:return countryName(d.country)==countryName(country);
  case CleanupKind::ExceptCountry:return countryName(d.country)!=countryName(country);
  case CleanupKind::MissingDistance:return !d.km||!std::isfinite(*d.km)||(includeCached&&d.gridSource!="received");
  case CleanupKind::MissingGrid:return d.identity.grid.empty();
 }return false;
}
Json SavedCleanup::json()const{return {{"kind",int(kind)},{"from",from},{"to",to},{"country",countryName(country)},{"include_cached",includeCached}};}
SavedCleanup SavedCleanup::fromJson(const Json& j){SavedCleanup s;s.kind=CleanupKind(j.at("kind").get<int>());s.from=j.value("from",int64_t(0));s.to=j.value("to",int64_t(0));s.country=countryName(j.value("country",""));s.includeCached=j.value("include_cached",false);if(!s.valid())throw std::runtime_error("Invalid saved-message cleanup criteria");return s;}
std::string SavedCleanup::description()const{
 switch(kind){
  case CleanupKind::Before:return "Receptions before "+localDateTime(from);
  case CleanupKind::After:return "Receptions after "+localDateTime(from);
  case CleanupKind::Between:return "Receptions from "+localDateTime(from)+" through "+localDateTime(to)+" (inclusive)";
  case CleanupKind::Country:return "Receptions from "+countryName(country);
  case CleanupKind::ExceptCountry:return "Keep "+countryName(country)+"; delete receptions from all other countries, including unknown countries";
  case CleanupKind::MissingDistance:return includeCached?"Receptions without a distance from a received grid (missing and cached distances)":"Receptions with no numeric distance; keep cached distances";
  case CleanupKind::MissingGrid:return "Receptions with no saved grid information";
 }return {};
}
}
