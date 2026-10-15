// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <set>
namespace dx {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db,const std::string& sql){sqlite3_stmt* s=nullptr;if(sqlite3_prepare_v2(db,sql.c_str(),-1,&s,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));return {s,sqlite3_finalize};}
int step(sqlite3* db,sqlite3_stmt* s){int rc=sqlite3_step(s);if(rc!=SQLITE_ROW&&rc!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db));return rc;}
std::string text(sqlite3_stmt* s,int i){auto p=sqlite3_column_text(s,i);return p?reinterpret_cast<const char*>(p):"";}
Decode decode(sqlite3_stmt* s,int i=0){return Decode::fromJson(Json::parse(text(s,i)));}
}
Stats Store::stats(int64_t now){
 Stats out;auto countries=prepare(db_,"SELECT value FROM entities WHERE kind='country' ORDER BY value");while(step(db_,countries.get())==SQLITE_ROW)out.heardCountries.push_back(text(countries.get(),0));out.countries=out.heardCountries.size();
 auto calls=prepare(db_,"SELECT count(*) FROM entities WHERE kind='call' AND value NOT LIKE '<%'");if(step(db_,calls.get())==SQLITE_ROW)out.stations=sqlite3_column_int64(calls.get(),0);out.messages=savedMessageCount();
 auto recent=prepare(db_,"SELECT count(DISTINCT call) FROM decodes WHERE utc BETWEEN ? AND ? AND call!='' AND call NOT LIKE '<%'");sqlite3_bind_int64(recent.get(),1,now-300);sqlite3_bind_int64(recent.get(),2,now+30);if(step(db_,recent.get())==SQLITE_ROW)out.recentStations=sqlite3_column_int64(recent.get(),0);
 auto farthest=prepare(db_,"SELECT payload FROM decodes WHERE km IS NOT NULL AND json_extract(payload,'$.grid_source')='received' ORDER BY km DESC,utc DESC,reception_id DESC LIMIT 1");if(step(db_,farthest.get())==SQLITE_ROW){auto d=decode(farthest.get());out.farthest=d.km.value_or(0);out.farthestCountry=d.country;out.farthestCall=d.identity.transmitter;out.farthestReception=std::move(d);}
 auto latest=prepare(db_,"SELECT d.payload FROM entities AS e JOIN decodes AS d ON d.reception_id=e.last_id WHERE e.kind='country' ORDER BY e.last_utc DESC,e.last_id DESC");while(step(db_,latest.get())==SQLITE_ROW)out.latestCountries.push_back(decode(latest.get()));
 auto last=prepare(db_,"SELECT d.payload FROM entities AS e JOIN decodes AS d ON d.reception_id=e.last_id WHERE e.kind='call' AND e.value NOT LIKE '<%' ORDER BY e.last_utc DESC,e.last_id DESC LIMIT 5");while(step(db_,last.get())==SQLITE_ROW)out.latest.push_back(decode(last.get()));
 auto first=prepare(db_,"SELECT value FROM entities WHERE kind='country' ORDER BY first_id DESC LIMIT 3");while(step(db_,first.get())==SQLITE_ROW)out.newestCountries.push_back(text(first.get(),0));
 out.activeSeconds=getSetting("active_seconds",0.).get<double>();return out;
}
Stats Store::sessionStats(int64_t now,int64_t afterRowId,int64_t since){
 if(afterRowId==0&&since==0){auto out=stats(now);auto far=prepare(db_,"SELECT payload FROM decodes WHERE km IS NOT NULL ORDER BY km DESC,utc DESC,reception_id DESC LIMIT 1");if(step(db_,far.get())==SQLITE_ROW){auto d=decode(far.get());out.farthest=d.km.value_or(0);out.farthestCountry=d.country;out.farthestCall=d.identity.transmitter;out.farthestReception=std::move(d);}return out;}
 auto version=prepare(db_,"PRAGMA data_version");step(db_,version.get());auto current=sqlite3_column_int64(version.get(),0);if(current!=sessionDataVersion_){invalidateSession();sessionDataVersion_=current;}
 if(sessionAfter_!=afterRowId||sessionSince_!=since){sessionAfter_=afterRowId;sessionSince_=since;sessionCursor_=afterRowId;sessionFarthestId_=0;session_={};sessionCalls_.clear();sessionCountries_.clear();sessionCountryFirst_.clear();}
 auto read=prepare(db_,"SELECT reception_id,payload FROM decodes WHERE reception_id>? ORDER BY reception_id");sqlite3_bind_int64(read.get(),1,sessionCursor_);
 // Only committed new arrivals are read. A failure invalidates the partial cache.
 try{while(step(db_,read.get())==SQLITE_ROW){auto id=sqlite3_column_int64(read.get(),0);auto d=decode(read.get(),1);sessionCursor_=id;if(d.source=="legacy"&&d.utc<since)continue;++session_.messages;
  auto latest=[&](auto& cache,const std::string& key){if(key.empty()||key.front()=='<')return;auto found=cache.find(key);if(found==cache.end()||std::make_pair(d.utc,id)>std::make_pair(found->second.second.utc,found->second.first))cache[key]={id,d};};
  latest(sessionCalls_,d.identity.transmitter);latest(sessionCountries_,d.country);if(!d.country.empty())sessionCountryFirst_.emplace(d.country,id);
  if(d.km&&(!session_.farthestReception||*d.km>session_.farthest||(*d.km==session_.farthest&&d.utc>=session_.farthestReception->utc))){session_.farthest=*d.km;session_.farthestCountry=d.country;session_.farthestCall=d.identity.transmitter;session_.farthestReception=d;sessionFarthestId_=id;}
 }}catch(...){invalidateSession();throw;}
 Stats out=session_;out.stations=sessionCalls_.size();out.countries=sessionCountries_.size();
 auto latest=[](const auto& cache,size_t limit){std::vector<const std::pair<int64_t,Decode>*> sorted;for(const auto& [key,value]:cache){(void)key;sorted.push_back(&value);}auto count=std::min(limit,sorted.size());std::partial_sort(sorted.begin(),sorted.begin()+count,sorted.end(),[](const auto* a,const auto* b){return std::make_pair(a->second.utc,a->first)>std::make_pair(b->second.utc,b->first);});std::vector<Decode> rows;for(size_t i=0;i<count;i++)rows.push_back(sorted[i]->second);return rows;};
 out.latest=latest(sessionCalls_,5);out.latestCountries=latest(sessionCountries_,sessionCountries_.size());
 std::vector<std::pair<int64_t,std::string>> first;for(const auto& [country,id]:sessionCountryFirst_)first.emplace_back(id,country);std::sort(first.rbegin(),first.rend());for(size_t i=0;i<std::min<size_t>(3,first.size());i++)out.newestCountries.push_back(first[i].second);
 auto recent=prepare(db_,"SELECT count(DISTINCT call) FROM decodes WHERE reception_id>? AND utc BETWEEN ? AND ? AND call!='' AND call NOT LIKE '<%' AND (coalesce(json_extract(payload,'$.source'),'legacy')!='legacy' OR utc>=?)");sqlite3_bind_int64(recent.get(),1,afterRowId);sqlite3_bind_int64(recent.get(),2,now-300);sqlite3_bind_int64(recent.get(),3,now+30);sqlite3_bind_int64(recent.get(),4,since);if(step(db_,recent.get())==SQLITE_ROW)out.recentStations=sqlite3_column_int64(recent.get(),0);return out;
}
void Store::expireSession(const std::vector<std::pair<int64_t,Decode>>& expired){
 if(sessionAfter_<0||expired.empty())return;
 try{
  std::set<int64_t> ids;std::set<std::string> calls,countries;for(const auto& [id,d]:expired){ids.insert(id);if(!d.identity.transmitter.empty()&&d.identity.transmitter.front()!='<')calls.insert(d.identity.transmitter);if(!d.country.empty())countries.insert(d.country);}session_.messages-=int64_t(expired.size());
  const std::string scope="reception_id>? AND reception_id<=? AND (coalesce(json_extract(payload,'$.source'),'legacy')!='legacy' OR utc>=?)";
  auto scoped=[&](const std::string& sql,const std::string* value=nullptr){auto s=prepare(db_,sql);int pos=1;if(value)sqlite3_bind_text(s.get(),pos++,value->c_str(),int(value->size()),SQLITE_TRANSIENT);sqlite3_bind_int64(s.get(),pos++,sessionAfter_);sqlite3_bind_int64(s.get(),pos++,sessionCursor_);sqlite3_bind_int64(s.get(),pos,sessionSince_);return s;};
  auto refresh=[&](auto& cache,const auto& values,const char* field,const char* index){for(const auto& value:values){auto found=cache.find(value);if(found==cache.end()||!ids.count(found->second.first))continue;auto s=scoped("SELECT reception_id,payload FROM decodes INDEXED BY "+std::string(index)+" WHERE "+field+"=? AND "+scope+" ORDER BY utc DESC,reception_id DESC LIMIT 1",&value);if(step(db_,s.get())==SQLITE_ROW)found->second={sqlite3_column_int64(s.get(),0),decode(s.get(),1)};else cache.erase(found);}};
  refresh(sessionCalls_,calls,"call","by_call_time");refresh(sessionCountries_,countries,"country","by_country");
  for(const auto& country:countries){auto found=sessionCountryFirst_.find(country);if(found==sessionCountryFirst_.end()||!ids.count(found->second))continue;auto s=scoped("SELECT reception_id FROM decodes INDEXED BY by_country_id WHERE country=? AND "+scope+" ORDER BY reception_id LIMIT 1",&country);if(step(db_,s.get())==SQLITE_ROW)found->second=sqlite3_column_int64(s.get(),0);else sessionCountryFirst_.erase(found);}
  if(ids.count(sessionFarthestId_)){auto s=scoped("SELECT reception_id,payload FROM decodes INDEXED BY by_distance WHERE "+scope+" AND km IS NOT NULL ORDER BY km DESC,utc DESC,reception_id DESC LIMIT 1");if(step(db_,s.get())==SQLITE_ROW){auto d=decode(s.get(),1);sessionFarthestId_=sqlite3_column_int64(s.get(),0);session_.farthest=d.km.value_or(0);session_.farthestCountry=d.country;session_.farthestCall=d.identity.transmitter;session_.farthestReception=std::move(d);}else{sessionFarthestId_=0;session_.farthest=0;session_.farthestCountry.clear();session_.farthestCall.clear();session_.farthestReception.reset();}}
 }catch(...){invalidateSession();throw;}
}
}
