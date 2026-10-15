// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "model.h"
#include <ctime>
namespace dx {
namespace {
bool sameTime(const tm& a,const tm& b){return a.tm_year==b.tm_year&&a.tm_mon==b.tm_mon&&a.tm_mday==b.tm_mday&&a.tm_hour==b.tm_hour&&a.tm_min==b.tm_min&&a.tm_sec==b.tm_sec;}
std::optional<tm> fields(const std::string& text){
 if(text.size()!=10&&text.size()!=19)return {};tm date{};
 auto end=strptime(text.c_str(),text.size()==10?"%Y-%m-%d":"%Y-%m-%d %H:%M:%S",&date);
 if(!end||*end||date.tm_year<70)return {};return date;
}
}
std::string localDateTime(int64_t timestamp,bool withZone){
 if(timestamp<=0)return "--";time_t epoch=timestamp;tm date{};if(!localtime_r(&epoch,&date))return "--";
 char stamp[64];if(!std::strftime(stamp,sizeof(stamp),withZone?"%Y-%m-%d %H:%M:%S %z":"%Y-%m-%d %H:%M:%S",&date))return "--";
 std::string result=stamp;if(withZone&&result.size()==25)result.insert(result.size()-2,":");return result;
}
std::optional<int64_t> localDateTimeParse(const std::string& text,bool endOfDay){
 auto parsed=fields(text);if(!parsed)return {};auto date=*parsed;date.tm_isdst=-1;auto epoch=mktime(&date);tm normalized{};
 if(epoch<0||!localtime_r(&epoch,&normalized)||!sameTime(*parsed,normalized))return {};
 // Repeated wall-clock hours are ambiguous; never silently choose a destructive cutoff.
 for(int dst:{0,1}){auto other=*parsed;other.tm_isdst=dst;auto candidate=mktime(&other);tm check{};
  if(candidate!=epoch&&localtime_r(&candidate,&check)&&sameTime(*parsed,check))return {};
 }
 if(text.size()==10&&endOfDay){auto next=*parsed;++next.tm_mday;next.tm_isdst=-1;auto tomorrow=mktime(&next);if(tomorrow<=epoch)return {};return int64_t(tomorrow)-1;}
 return int64_t(epoch);
}
std::optional<int64_t> restoredDateTimeEpoch(const Json& settings,const std::string& key){
 auto text=settings.value(key,"");auto epochKey=key+"_epoch";
 if(settings.contains(epochKey)&&settings[epochKey].is_number_integer()){auto epoch=settings[epochKey].get<int64_t>();if(epoch>0)return epoch;}
 if(settings.value("date_time_style",0)>=1)return localDateTimeParse(text,key=="custom_until");
 // Older custom ranges were entered as UTC. Preserve their instants on upgrade.
 auto date=fields(text);if(!date)return {};auto epoch=timegm(&*date);tm normalized{};
 if(epoch<0||!gmtime_r(&epoch,&normalized)||!sameTime(*date,normalized))return {};
 return int64_t(epoch);
}
std::string restoredLocalDateTime(const Json& settings,const std::string& key){
 auto epoch=restoredDateTimeEpoch(settings,key);return epoch?localDateTime(*epoch,false):settings.value(key,"");
}
}
