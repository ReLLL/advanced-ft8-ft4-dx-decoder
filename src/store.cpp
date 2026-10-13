// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include <memory>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <set>
#include <tuple>
namespace dx {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db,const std::string& sql){sqlite3_stmt* raw=nullptr;if(sqlite3_prepare_v2(db,sql.c_str(),-1,&raw,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));return {raw,sqlite3_finalize};}
void bindText(sqlite3_stmt* s,int pos,const std::string& v){sqlite3_bind_text(s,pos,v.c_str(),int(v.size()),SQLITE_TRANSIENT);}
std::string column(sqlite3_stmt* s,int i){const auto* p=sqlite3_column_text(s,i);return p?reinterpret_cast<const char*>(p):"";}
int step(sqlite3* db,sqlite3_stmt* s){int code=sqlite3_step(s);if(code!=SQLITE_ROW&&code!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db));return code;}
void sqlMatch(sqlite3_context* c,int,sqlite3_value** v){const auto* a=sqlite3_value_text(v[0]);const auto* b=sqlite3_value_text(v[1]);sqlite3_result_int(c,matches(a?reinterpret_cast<const char*>(a):"",b?reinterpret_cast<const char*>(b):""));}
void sqlBand(sqlite3_context* c,int,sqlite3_value** v){auto band=bandName(sqlite3_value_int64(v[0]));sqlite3_result_text(c,band.c_str(),-1,SQLITE_TRANSIENT);}
std::string csvField(std::string s){size_t p=0;while((p=s.find('"',p))!=std::string::npos){s.insert(p,1,'"');p+=2;}return '"'+s+'"';}
}
void Store::exec(const std::string& sql){char* error=nullptr;if(sqlite3_exec(db_,sql.c_str(),nullptr,nullptr,&error)!=SQLITE_OK){std::string message=error?error:"SQLite error";sqlite3_free(error);throw std::runtime_error(message);}}
Store::Store(const std::filesystem::path& file){
 if(sqlite3_open_v2(file.c_str(),&db_,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_NOMUTEX,nullptr)!=SQLITE_OK){std::string error=sqlite3_errmsg(db_);sqlite3_close(db_);db_=nullptr;throw std::runtime_error(error);}
 sqlite3_busy_timeout(db_,2000);sqlite3_create_function(db_,"dx_match",2,SQLITE_UTF8|SQLITE_DETERMINISTIC,nullptr,sqlMatch,nullptr,nullptr);
 sqlite3_create_function(db_,"dx_band",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,nullptr,sqlBand,nullptr,nullptr);
 exec("PRAGMA journal_mode=WAL;PRAGMA synchronous=FULL;"
 "CREATE TABLE IF NOT EXISTS decodes(key TEXT PRIMARY KEY,utc INTEGER NOT NULL,dial INTEGER NOT NULL,mode TEXT NOT NULL,call TEXT NOT NULL,called TEXT,country TEXT,continent TEXT,grid TEXT,type TEXT,band TEXT,snr INTEGER,dt REAL,km REAL,message TEXT,payload TEXT NOT NULL);"
 "CREATE INDEX IF NOT EXISTS by_time ON decodes(utc);CREATE INDEX IF NOT EXISTS by_station ON decodes(call,dial,mode,utc);"
 "CREATE INDEX IF NOT EXISTS by_message ON decodes(call,dial,mode,message,utc);"
 "CREATE INDEX IF NOT EXISTS by_call_message ON decodes(call,message,utc);"
 "CREATE INDEX IF NOT EXISTS by_country ON decodes(country,utc);CREATE INDEX IF NOT EXISTS by_grid ON decodes(grid,utc);CREATE INDEX IF NOT EXISTS by_distance ON decodes(km DESC,utc DESC);"
 "CREATE TABLE IF NOT EXISTS stations(call TEXT,dial INTEGER,mode TEXT,utc INTEGER,count INTEGER,payload TEXT,PRIMARY KEY(call,dial,mode));"
 "CREATE TABLE IF NOT EXISTS heard(kind TEXT,value TEXT,utc INTEGER,PRIMARY KEY(kind,value));"
 "CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY,payload TEXT);"
 "CREATE TABLE IF NOT EXISTS imports(path TEXT PRIMARY KEY,cursor INTEGER);"
 "CREATE TABLE IF NOT EXISTS counters(key TEXT PRIMARY KEY,value INTEGER);INSERT OR IGNORE INTO counters VALUES('messages',0);");
 if(getSetting("country_names",0).get<int>()<1){exec("BEGIN IMMEDIATE");try{
  exec("UPDATE decodes SET country='Germany',payload=json_set(payload,'$.country','Germany') WHERE country='Fed. Rep. of Germany';"
   "UPDATE stations SET payload=json_set(payload,'$.country','Germany') WHERE json_extract(payload,'$.country')='Fed. Rep. of Germany';"
   "INSERT INTO heard SELECT kind,'Germany',utc FROM heard WHERE kind='country' AND value='Fed. Rep. of Germany' ON CONFLICT(kind,value) DO UPDATE SET utc=min(utc,excluded.utc);"
   "DELETE FROM heard WHERE kind='country' AND value='Fed. Rep. of Germany';");
  auto farthest=getSetting("farthest_country",Json(""));if(farthest.is_string())setSetting("farthest_country",countryName(farthest.get<std::string>()));
  setSetting("country_names",1);exec("COMMIT");
 }catch(...){exec("ROLLBACK");throw;}}
}
Store::~Store(){if(db_)sqlite3_close(db_);}
Json Store::getSetting(const std::string& key,const Json& fallback){auto s=prepare(db_,"SELECT payload FROM settings WHERE key=?");bindText(s.get(),1,key);if(step(db_,s.get())!=SQLITE_ROW)return fallback;auto j=Json::parse(column(s.get(),0),nullptr,false);return j.is_discarded()?fallback:j;}
void Store::setSetting(const std::string& key,const Json& value){auto s=prepare(db_,"INSERT OR REPLACE INTO settings VALUES(?,?)");bindText(s.get(),1,key);bindText(s.get(),2,value.dump());step(db_,s.get());}
std::vector<std::pair<int64_t,Decode>> Store::logRows(int64_t after,int limit){auto s=prepare(db_,"SELECT rowid,payload FROM decodes WHERE rowid>? ORDER BY rowid LIMIT ?");sqlite3_bind_int64(s.get(),1,after);sqlite3_bind_int(s.get(),2,limit);std::vector<std::pair<int64_t,Decode>> out;while(step(db_,s.get())==SQLITE_ROW)out.push_back({sqlite3_column_int64(s.get(),0),Decode::fromJson(Json::parse(column(s.get(),1)))});return out;}
int64_t Store::lastReceptionId(){auto s=prepare(db_,"SELECT coalesce(max(rowid),0) FROM decodes");return step(db_,s.get())==SQLITE_ROW?sqlite3_column_int64(s.get(),0):0;}
bool Store::insert(Decode& d,Point home,const std::function<std::pair<std::string,std::string>(const std::string&)>& country){
 d.identity=meaning(d.message);auto& i=d.identity;
 auto resolve=[&](std::string& value){std::smatch match;static const std::regex hash("<([0-2]):([0-9]{1,7})>");if(!std::regex_match(value,match,hash))return;int bits=match[1]=="0"?10:match[1]=="1"?12:22;uint32_t wanted=uint32_t(std::stoul(match[2]));
  auto s=prepare(db_,"SELECT DISTINCT call FROM decodes WHERE utc BETWEEN ? AND ? AND call!='' AND call NOT LIKE '<%'");sqlite3_bind_int64(s.get(),1,d.utc-180);sqlite3_bind_int64(s.get(),2,d.utc);std::string candidate;bool ambiguous=false;
  while(step(db_,s.get())==SQLITE_ROW){auto call=column(s.get(),0);if(callHash(call,bits)==wanted){if(!candidate.empty()&&candidate!=call)ambiguous=true;candidate=call;}}
  if(!ambiguous&&!candidate.empty()){value=candidate;d.identitySource="unique-recent-hash";}
 };resolve(i.transmitter);resolve(i.called);
 if(country&&!i.transmitter.empty()&&i.transmitter.front()!='<'){auto c=country(i.transmitter);d.country=c.first;d.continent=c.second;}
 d.country=countryName(d.country);
 if(!i.grid.empty()){d.gridSource="received";d.gridUtc=d.utc;d.position=gridPoint(i.grid);}
 else if(!i.transmitter.empty()&&i.transmitter.front()!='<'){
  auto s=prepare(db_,"SELECT payload FROM decodes WHERE call=? AND utc<=? AND utc>=? AND grid!='' AND json_extract(payload,'$.grid_source')='received' ORDER BY utc DESC LIMIT 1");bindText(s.get(),1,i.transmitter);sqlite3_bind_int64(s.get(),2,d.utc);sqlite3_bind_int64(s.get(),3,d.utc-21600);
  if(step(db_,s.get())==SQLITE_ROW){auto last=Decode::fromJson(Json::parse(column(s.get(),0)));if(last.gridSource=="received"){i.grid=last.identity.grid;d.gridSource="cached";d.gridUtc=last.utc;d.position=gridPoint(i.grid);}}
 }
 if(d.position){auto b=distanceBearing(home,*d.position);d.km=b.first;d.bearing=b.second;}
 auto key=d.key();auto duplicate=prepare(db_,"SELECT 1 FROM decodes WHERE key=?");bindText(duplicate.get(),1,key);if(step(db_,duplicate.get())==SQLITE_ROW)return false;
 exec("BEGIN IMMEDIATE");try{
  auto heard=[&](const std::string& kind,const std::string& value){if(value.empty()||value.front()=='<')return false;auto s=prepare(db_,"INSERT OR IGNORE INTO heard VALUES(?,?,?)");bindText(s.get(),1,kind);bindText(s.get(),2,value);sqlite3_bind_int64(s.get(),3,d.utc);step(db_,s.get());return sqlite3_changes(db_)==1;};
  d.firstCall=heard("call",i.transmitter);d.firstGrid=d.gridSource=="received"&&heard("grid",i.grid);d.firstCountry=heard("country",d.country);
  d.distanceRecord=false;if(d.km&&d.gridSource=="received"&&*d.km>getSetting("farthest_km",0.).get<double>()){d.distanceRecord=true;setSetting("farthest_km",*d.km);setSetting("farthest_country",d.country);}
  auto payload=d.json().dump();auto s=prepare(db_,"INSERT INTO decodes VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  bindText(s.get(),1,key);sqlite3_bind_int64(s.get(),2,d.utc);sqlite3_bind_int64(s.get(),3,d.dial);bindText(s.get(),4,d.mode);bindText(s.get(),5,i.transmitter);bindText(s.get(),6,i.called);bindText(s.get(),7,d.country);bindText(s.get(),8,d.continent);bindText(s.get(),9,i.grid);bindText(s.get(),10,i.type);bindText(s.get(),11,bandName(d.dial));sqlite3_bind_int(s.get(),12,d.snr);if(d.dt)sqlite3_bind_double(s.get(),13,*d.dt);if(d.km)sqlite3_bind_double(s.get(),14,*d.km);bindText(s.get(),15,d.message);bindText(s.get(),16,payload);step(db_,s.get());
  if(!i.transmitter.empty()){
   auto a=prepare(db_,"INSERT INTO stations VALUES(?,?,?,?,1,?) ON CONFLICT(call,dial,mode) DO UPDATE SET count=count+1,payload=CASE WHEN excluded.utc>=utc THEN excluded.payload ELSE payload END,utc=max(utc,excluded.utc)");bindText(a.get(),1,i.transmitter);sqlite3_bind_int64(a.get(),2,d.dial);bindText(a.get(),3,d.mode);sqlite3_bind_int64(a.get(),4,d.utc);bindText(a.get(),5,payload);step(db_,a.get());
  }
  if(d.km&&d.gridSource=="received"){
   auto previous=getSetting("farthest_km",0.).get<double>();if(*d.km>previous)setSetting("farthest_km",*d.km);
  }
  exec("UPDATE counters SET value=value+1 WHERE key='messages';COMMIT");return true;
 }catch(...){exec("ROLLBACK");throw;}
}
std::vector<Row> Store::rows(const Query& q,int offset,int limit,int64_t* countOnly){
 const auto& f=q.filter;
 bool empty=f.json()==Filter{}.json();std::vector<std::string> values;
 std::string where="utc>=? AND utc<=?";
 if(q.afterRowId>0)where+=" AND rowid>?";
 auto match=[&](const std::string& field,const std::string& patterns,bool exclude=false){if(patterns.empty())return;where+=(exclude?" AND NOT dx_match(":" AND dx_match(")+field+",?)";values.push_back(patterns);};
 match("call",f.call);match("called",f.called);match("country",f.country);match("continent",f.continent);match("grid",f.grid);match("band",f.band);match("mode",f.mode);match("CAST(dial AS TEXT)",f.frequency);match("type",f.type);
 match("call",f.excludeCall,true);match("called",f.excludeCalled,true);match("country",f.excludeCountry,true);match("grid",f.excludeGrid,true);
 if(!f.prefix.empty()){std::stringstream in(f.prefix);std::string p,patterns;while(std::getline(in,p,',')){if(!patterns.empty())patterns+=',';patterns+=p+'*';}match("call",patterns);}
 if(!f.search.empty()){where+=" AND instr(upper(message||' '||country||' '||call),?)>0";values.push_back(upper(f.search));}
 if(!f.text.empty()){where+=" AND instr(upper(message),?)>0";values.push_back(upper(f.text));}
 if(f.cqOnly)where+=" AND type='CQ'";if(f.hasGrid)where+=" AND grid!=''";if(f.first)where+=" AND json_extract(payload,'$.first_call')=1";
 if(f.messageKind==MessageKind::GeneralCall)where+=" AND called IN ('CQ','QRZ')";
 if(f.messageKind==MessageKind::Exchange)where+=" AND call!='' AND called NOT IN ('','CQ','QRZ','DE')";
 if(f.messageKind==MessageKind::Other)where+=" AND called NOT IN ('CQ','QRZ') AND (call='' OR called IN ('','DE'))";
 where+=" AND snr>="+std::to_string(f.minSnr);
 if(f.maxDt<30)where+=" AND abs(dt)<="+std::to_string(f.maxDt);
 if(f.minKm>0||f.maxKm<40080)where+=" AND km BETWEEN "+std::to_string(f.minKm)+" AND "+std::to_string(f.maxKm);
 const bool cached=q.since==0&&q.until==INT64_MAX&&q.afterRowId==0&&empty&&!q.messages&&!q.uniqueCalls;
 const char* fields[]={"timestamp","transmitter","country","km","snr","grid","mode","frequency","frequency","audio_hz","dt","bearing","","called","type","message"};
 int sort=std::clamp(q.sort,0,17);std::string order=sort==17?"station_first_heard":sort==16?"first_heard":sort==12?"row_count":"json_extract(payload,'$."+std::string(fields[sort])+"')";
 const char* indexed[]={"utc","call","country","km","snr","grid","mode","band","dial","","dt","","","called","type","message"};
 if(sort<16&&*indexed[sort]&&(!cached||sort==0||sort==1||sort==6||sort==8))order=indexed[sort];
 if(sort==7)order="dx_band(dial)";
 std::string sql;
 if(cached)sql="SELECT *,count AS row_count FROM stations";
 else if(q.messages)sql="SELECT *,1 AS row_count FROM decodes WHERE "+where;
 else sql="SELECT *,count(*) AS row_count FROM decodes WHERE "+where+" GROUP BY CASE WHEN call='' THEN key ELSE call END"+(q.uniqueCalls?std::string{}:",dial,mode")+" HAVING max(utc)";
 if(countOnly)sql="SELECT count(*) FROM ("+sql+")";
 else {
  // Session/clear cutoffs are independent of rolling time and display filters.
  auto cutoff=" AND first.utc>="+std::to_string(q.firstSince)+" AND first.rowid>"+std::to_string(q.firstAfterId);
  auto first="coalesce((SELECT min(first.utc) FROM decodes AS first WHERE first.call=shown.call AND first.message=json_extract(shown.payload,'$.message')"+cutoff+"),shown.utc)";
  auto station="(SELECT min(first.utc) FROM decodes AS first WHERE first.call=shown.call AND shown.call!='' AND shown.call NOT LIKE '<%'"+cutoff+")";
  sql="WITH shown AS ("+sql+") SELECT payload,row_count,"+first+" AS first_heard,"+station+" AS station_first_heard FROM shown ORDER BY ("+order+" IS NULL) ASC,"+order+(q.descending?" DESC":" ASC")+",utc DESC,call ASC,dial ASC,mode ASC LIMIT ? OFFSET ?";
 }
 auto s=prepare(db_,sql);int pos=1;
 if(!cached){sqlite3_bind_int64(s.get(),pos++,q.since);sqlite3_bind_int64(s.get(),pos++,q.until);if(q.afterRowId>0)sqlite3_bind_int64(s.get(),pos++,q.afterRowId);for(const auto& v:values)bindText(s.get(),pos++,v);}
 if(countOnly){*countOnly=step(db_,s.get())==SQLITE_ROW?sqlite3_column_int64(s.get(),0):0;return {};}
 sqlite3_bind_int(s.get(),pos++,limit);sqlite3_bind_int(s.get(),pos,offset);std::vector<Row> rows;
 while(step(db_,s.get())==SQLITE_ROW){auto d=Decode::fromJson(Json::parse(column(s.get(),0)));rows.push_back({std::move(d),sqlite3_column_int64(s.get(),1),sqlite3_column_int64(s.get(),2),sqlite3_column_int64(s.get(),3)});}return rows;
}
std::vector<Decode> Store::history(const std::string& call,int64_t dial,const std::string& mode,int offset){auto s=prepare(db_,"SELECT payload FROM decodes WHERE call=? AND dial=? AND mode=? ORDER BY utc DESC LIMIT 500 OFFSET ?");bindText(s.get(),1,call);sqlite3_bind_int64(s.get(),2,dial);bindText(s.get(),3,mode);sqlite3_bind_int(s.get(),4,offset);std::vector<Decode> out;while(step(db_,s.get())==SQLITE_ROW)out.push_back(Decode::fromJson(Json::parse(column(s.get(),0))));return out;}
std::vector<Decode> Store::uniqueMessages(const Decode& station,const Query& view){
 auto s=prepare(db_,"SELECT payload FROM decodes WHERE call=? AND dial=? AND mode=? AND utc BETWEEN ? AND ? AND rowid>? GROUP BY message HAVING max(utc) ORDER BY utc DESC,message LIMIT 5");
 bindText(s.get(),1,station.identity.transmitter);sqlite3_bind_int64(s.get(),2,station.dial);bindText(s.get(),3,station.mode);sqlite3_bind_int64(s.get(),4,view.since);sqlite3_bind_int64(s.get(),5,view.until);sqlite3_bind_int64(s.get(),6,view.afterRowId);
 std::vector<Decode> out;while(step(db_,s.get())==SQLITE_ROW)out.push_back(Decode::fromJson(Json::parse(column(s.get(),0))));return out;
}
Stats Store::stats(int64_t now){Stats out;
 auto countries=prepare(db_,"SELECT value FROM heard WHERE kind='country' ORDER BY value");while(step(db_,countries.get())==SQLITE_ROW)out.heardCountries.push_back(column(countries.get(),0));
 auto s=prepare(db_,"SELECT kind,count(*) FROM heard GROUP BY kind");while(step(db_,s.get())==SQLITE_ROW){auto kind=column(s.get(),0);if(kind=="country")out.countries=sqlite3_column_int64(s.get(),1);if(kind=="call")out.stations=sqlite3_column_int64(s.get(),1);}
 auto c=prepare(db_,"SELECT value FROM counters WHERE key='messages'");if(step(db_,c.get())==SQLITE_ROW)out.messages=sqlite3_column_int64(c.get(),0);
 auto recent=prepare(db_,"SELECT count(DISTINCT call) FROM decodes WHERE utc>=? AND utc<=? AND call!='' AND call NOT LIKE '<%'");sqlite3_bind_int64(recent.get(),1,now-300);sqlite3_bind_int64(recent.get(),2,now+30);if(step(db_,recent.get())==SQLITE_ROW)out.recentStations=sqlite3_column_int64(recent.get(),0);
 out.farthest=getSetting("farthest_km",0.).get<double>();
 auto farthestCountry=getSetting("farthest_country",Json(nullptr));if(farthestCountry.is_null()){auto farthest=prepare(db_,"SELECT country FROM decodes WHERE km IS NOT NULL AND json_extract(payload,'$.grid_source')='received' ORDER BY km DESC,utc DESC LIMIT 1");out.farthestCountry=step(db_,farthest.get())==SQLITE_ROW?column(farthest.get(),0):"";setSetting("farthest_country",out.farthestCountry);}else out.farthestCountry=farthestCountry.get<std::string>();
 auto farthestCall=prepare(db_,"SELECT call FROM decodes WHERE km IS NOT NULL AND json_extract(payload,'$.grid_source')='received' ORDER BY km DESC,utc DESC LIMIT 1");if(step(db_,farthestCall.get())==SQLITE_ROW)out.farthestCall=column(farthestCall.get(),0);
 auto last=prepare(db_,"SELECT payload FROM stations WHERE call!='' AND call NOT LIKE '<%' GROUP BY call HAVING max(utc) ORDER BY utc DESC LIMIT 5");while(step(db_,last.get())==SQLITE_ROW)out.latest.push_back(Decode::fromJson(Json::parse(column(last.get(),0))));
 out.activeSeconds=getSetting("active_seconds",0.).get<double>();return out;
}
Stats Store::sessionStats(int64_t now,int64_t afterRowId,int64_t since){
 Stats out;const std::string where="rowid>? AND (coalesce(json_extract(payload,'$.source'),'legacy')!='legacy' OR utc>=?)";
 auto query=[&](const std::string& sql){auto s=prepare(db_,sql);sqlite3_bind_int64(s.get(),1,afterRowId);sqlite3_bind_int64(s.get(),2,since);return s;};
 auto totals=query("SELECT count(*),count(DISTINCT CASE WHEN call!='' AND call NOT LIKE '<%' THEN call END),count(DISTINCT nullif(country,'')) FROM decodes WHERE "+where);
 if(step(db_,totals.get())==SQLITE_ROW){out.messages=sqlite3_column_int64(totals.get(),0);out.stations=sqlite3_column_int64(totals.get(),1);out.countries=sqlite3_column_int64(totals.get(),2);}
 auto recent=query("SELECT count(DISTINCT call) FROM decodes WHERE "+where+" AND utc>=? AND utc<=? AND call!='' AND call NOT LIKE '<%'");sqlite3_bind_int64(recent.get(),3,now-300);sqlite3_bind_int64(recent.get(),4,now+30);if(step(db_,recent.get())==SQLITE_ROW)out.recentStations=sqlite3_column_int64(recent.get(),0);
 auto farthest=query("SELECT km,country,call FROM decodes WHERE "+where+" AND km IS NOT NULL ORDER BY km DESC,utc DESC LIMIT 1");if(step(db_,farthest.get())==SQLITE_ROW){out.farthest=sqlite3_column_double(farthest.get(),0);out.farthestCountry=column(farthest.get(),1);out.farthestCall=column(farthest.get(),2);}
 auto first=query("SELECT key FROM decodes WHERE "+where+" AND country!='' GROUP BY country HAVING min(rowid)");while(step(db_,first.get())==SQLITE_ROW)out.firstCountryKeys.push_back(column(first.get(),0));
 auto last=query("SELECT payload FROM decodes WHERE "+where+" AND call!='' AND call NOT LIKE '<%' GROUP BY call HAVING max(utc) ORDER BY utc DESC LIMIT 5");while(step(db_,last.get())==SQLITE_ROW)out.latest.push_back(Decode::fromJson(Json::parse(column(last.get(),0))));return out;
}
int64_t Store::importBridge(const std::filesystem::path& path,Point home,const std::function<std::pair<std::string,std::string>(const std::string&)>& country){
 if(!std::filesystem::exists(path))return 0;sqlite3* source=nullptr;int code=sqlite3_open_v2(path.c_str(),&source,SQLITE_OPEN_READONLY,nullptr);std::unique_ptr<sqlite3,decltype(&sqlite3_close)> owner(source,sqlite3_close);if(code!=SQLITE_OK)throw std::runtime_error("Legacy history unavailable");
 sqlite3_busy_timeout(source,2000);auto cursor=getSetting("import_bridge_cursor",0).get<int64_t>();auto s=prepare(source,"SELECT rowid,payload FROM decodes WHERE rowid>? ORDER BY rowid LIMIT 500");sqlite3_bind_int64(s.get(),1,cursor);int64_t added=0;
 while(step(source,s.get())==SQLITE_ROW){auto j=Json::parse(column(s.get(),1));j["source"]="legacy";auto d=Decode::fromJson(j);if(insert(d,home,country))++added;cursor=sqlite3_column_int64(s.get(),0);}
 setSetting("import_bridge_cursor",cursor);return added;
}
void Store::exportRows(const Query& q,const std::filesystem::path& path,bool csv){std::ofstream file(path);if(!file)throw std::runtime_error("Export file unavailable");if(csv)file<<"UTC,Call,Called,Country,Grid,Mode,DialHz,AudioHz,SNRdB,DT,ApproxKm,Message\n";else file<<"[\n";
 int offset=0;bool comma=false;for(;;){auto page=rows(q,offset,1000);if(page.empty())break;for(auto& row:page){auto& d=row.decode;if(csv)file<<d.utc<<','<<csvField(d.identity.transmitter)<<','<<csvField(d.identity.called)<<','<<csvField(d.country)<<','<<csvField(d.identity.grid)<<','<<d.mode<<','<<d.dial<<','<<d.audio<<','<<d.snr<<','<<(d.dt?std::to_string(*d.dt):"")<<','<<(d.km?std::to_string(*d.km):"")<<','<<csvField(d.message)<<'\n';else{if(comma)file<<",\n";file<<d.json().dump();comma=true;}}offset+=int(page.size());}if(!csv)file<<"\n]\n";if(!file)throw std::runtime_error("Export write failed");}
void Store::reclassifyCountries(const std::function<std::pair<std::string,std::string>(const std::string&)>& country){
 if(getSetting("country_schema",0).get<int>()>=3)return;exec("BEGIN IMMEDIATE");try{
  exec("DELETE FROM heard WHERE kind='country'");auto read=prepare(db_,"SELECT key,payload FROM decodes ORDER BY utc");auto write=prepare(db_,"UPDATE decodes SET country=?,continent=?,payload=? WHERE key=?");
  while(step(db_,read.get())==SQLITE_ROW){auto d=Decode::fromJson(Json::parse(column(read.get(),1)));auto result=country(d.identity.transmitter);d.country=countryName(result.first);d.continent=result.second;d.firstCountry=false;
   if(!d.country.empty()){auto h=prepare(db_,"INSERT OR IGNORE INTO heard VALUES('country',?,?)");bindText(h.get(),1,d.country);sqlite3_bind_int64(h.get(),2,d.utc);step(db_,h.get());d.firstCountry=sqlite3_changes(db_)==1;}
   sqlite3_reset(write.get());sqlite3_clear_bindings(write.get());bindText(write.get(),1,d.country);bindText(write.get(),2,d.continent);bindText(write.get(),3,d.json().dump());bindText(write.get(),4,column(read.get(),0));step(db_,write.get());
  }
  exec("UPDATE stations SET payload=(SELECT payload FROM decodes WHERE decodes.call=stations.call AND decodes.dial=stations.dial AND decodes.mode=stations.mode ORDER BY utc DESC LIMIT 1)");setSetting("country_schema",3);exec("COMMIT");
 }catch(...){exec("ROLLBACK");throw;}
}
std::vector<ExchangePath> Store::exchangePaths(const Query& query){
 if(query.pathCount<=0||query.filter.messageKind==MessageKind::GeneralCall||query.filter.messageKind==MessageKind::Other)return {};
 auto q=query;q.messages=true;q.sort=0;q.descending=true;q.filter.messageKind=MessageKind::Exchange;auto recent=rows(q,0,1000);std::vector<ExchangePath> paths;
 auto last=prepare(db_,"SELECT coalesce(max(utc),0) FROM decodes WHERE utc BETWEEN ? AND ?");sqlite3_bind_int64(last.get(),1,query.since);sqlite3_bind_int64(last.get(),2,query.until);step(db_,last.get());auto end=sqlite3_column_int64(last.get(),0);
 std::set<std::tuple<std::string,std::string,int64_t,std::string>> seen;
 for(const auto& row:recent){auto& d=row.decode;if(d.identity.transmitter.empty()||d.identity.called.empty()||d.identity.called=="CQ"||d.identity.called.front()=='<'||d.identity.transmitter.front()=='<')continue;
  if(d.identity.transmitter==d.identity.called)continue;auto pair=std::minmax(d.identity.transmitter,d.identity.called);auto key=std::make_tuple(pair.first,pair.second,d.dial,d.mode);if(seen.count(key))continue;
  auto location=[&](const std::string& call)->std::optional<Decode>{auto s=prepare(db_,"SELECT payload FROM decodes WHERE call=? AND utc BETWEEN ? AND ? AND grid!='' AND json_extract(payload,'$.grid_source')='received' ORDER BY utc DESC LIMIT 1");bindText(s.get(),1,call);sqlite3_bind_int64(s.get(),2,end-21600);sqlite3_bind_int64(s.get(),3,end);if(step(db_,s.get())!=SQLITE_ROW)return {};return Decode::fromJson(Json::parse(column(s.get(),0)));};
  auto from=location(d.identity.transmitter),to=location(d.identity.called);if(!from||!to||!from->position||!to->position)continue;
  seen.insert(key);paths.push_back({d.identity.transmitter,d.identity.called,d.utc,from->utc,to->utc,greatCircle(*from->position,*to->position)});if(int(paths.size())>=query.pathCount)break;
 }return paths;
}
}
