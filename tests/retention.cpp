// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
extern char** environ;
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
namespace {
dx::Decode reception(int64_t utc,const std::string& call,const std::string& source="live"){dx::Decode d;d.utc=utc;d.slotMs=utc*1000;d.dial=7074000;d.mode="FT8";d.snr=-12;d.message="CQ "+call+" FN42";d.source=source;return d;}
int64_t scalar(sqlite3* db,const char* sql){sqlite3_stmt* s=nullptr;if(sqlite3_prepare_v2(db,sql,-1,&s,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));int rc=sqlite3_step(s);if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));auto n=sqlite3_column_int64(s,0);sqlite3_finalize(s);return n;}
bool entities(sqlite3* db){return scalar(db,"SELECT count(*) FROM entities AS e WHERE total!=(SELECT count(*) FROM decodes WHERE CASE e.kind WHEN 'call' THEN call=e.value WHEN 'country' THEN country=e.value ELSE call='' AND message=e.value END) OR first_utc!=(SELECT min(utc) FROM decodes WHERE CASE e.kind WHEN 'call' THEN call=e.value WHEN 'country' THEN country=e.value ELSE call='' AND message=e.value END) OR first_id!=(SELECT min(reception_id) FROM decodes WHERE CASE e.kind WHEN 'call' THEN call=e.value WHEN 'country' THEN country=e.value ELSE call='' AND message=e.value END) OR last_id!=(SELECT reception_id FROM decodes WHERE CASE e.kind WHEN 'call' THEN call=e.value WHEN 'country' THEN country=e.value ELSE call='' AND message=e.value END ORDER BY utc DESC,reception_id DESC LIMIT 1)")==0;}
}
int main(int argc,char** argv){
 // exec a fresh runtime: macOS logging inside SQLite can crash in a fork-only child.
 if(argc==3&&std::string(argv[1])=="--crash-writer"){
  sqlite3* db=nullptr;if(sqlite3_open_v2(argv[2],&db,SQLITE_OPEN_READWRITE,nullptr)!=SQLITE_OK)_exit(2);
  int rc=sqlite3_exec(db,"BEGIN IMMEDIATE;DELETE FROM decodes;UPDATE counters SET value=0 WHERE key='messages'",nullptr,nullptr,nullptr);
  _exit(rc==SQLITE_OK?0:3);
 }
 if(argc!=2)return 2;
 CHECK(dx::defaultHistoryLimit==100000&&dx::historyCleanupTarget(dx::defaultHistoryLimit)==95000&&dx::historyCheckIntervalMs==1800000);
 auto root=std::filesystem::temp_directory_path()/("dx-retention-"+std::to_string(getpid()));std::filesystem::create_directory(root);TestFolderCleanup cleanup(root);auto file=root/"history.sqlite3";dx::Point home{39,33};auto country=[](const std::string& call){return std::make_pair(call=="DL1ABC"?"Germany":"United States","NA");};int64_t boundary=0;
 // Custom targets persist across restarts and protect later arrivals.
 int64_t customBoundary=0;
 {dx::Store store(root/"custom.sqlite3");for(int i=0;i<120;i++){auto d=reception(5000+i,"K1ABC");CHECK(store.insert(d,home,country));}customBoundary=store.lastReceptionId();CHECK(store.retain(100,16,customBoundary,80)==16);auto later=reception(4999,"K2ABC");CHECK(store.insert(later,home,country));}
 {dx::Store store(root/"custom.sqlite3");for(int i=0;i<10&&store.retentionState().running;i++)store.retain(100,8,customBoundary,80);CHECK(store.savedMessageCount()==81&&!store.retentionState().running);auto rows=store.logRows(customBoundary,10);CHECK(rows.size()==1&&rows[0].second.identity.transmitter=="K2ABC");store.setSetting("ui",{{"history_max_records",20000},{"history_trim_to",12000}});CHECK(store.retentionState().target==12000);store.setSetting("ui",{{"history_max_records",10000},{"history_trim_to",50000}});CHECK(store.retentionState().target==9999);}
 {dx::Store store(root/"changed-target.sqlite3");for(int i=0;i<30;i++){auto d=reception(6000+i,"K1ABC");CHECK(store.insert(d,home,country));}CHECK(store.retain(20,4,INT64_MAX,10)==4);for(int i=0;i<10&&store.retentionState().running;i++)store.retain(20,4,INT64_MAX,15);CHECK(store.savedMessageCount()==15&&!store.retentionState().running);}
 {dx::Store store(file);for(int i=0;i<100;i++){auto d=reception(1000+(i*7)%100,i%2?"DL1ABC":"K1ABC");CHECK(store.insert(d,home,country));}boundary=store.lastReceptionId();CHECK(store.savedMessageCount()==100);auto prior=store.sessionStats(1200,5,0);CHECK(prior.messages==95);auto newer=reception(1500,"K2ABC");CHECK(store.insert(newer,home,country));CHECK(store.sessionStats(1501,5,0).messages==96);
  sqlite3* db=nullptr;CHECK(sqlite3_open(file.c_str(),&db)==SQLITE_OK);CHECK(entities(db));CHECK(scalar(db,"PRAGMA auto_vacuum")==2);
  // Force failure after DELETE triggers ran: every reception, summary, watermark and job must roll back.
  CHECK(sqlite3_exec(db,"CREATE TRIGGER fail_retention BEFORE UPDATE ON counters WHEN NEW.key='messages' BEGIN SELECT RAISE(ABORT,'injected retention interruption');END",nullptr,nullptr,nullptr)==SQLITE_OK);
  bool failed=false;try{store.retain(50,16,boundary);}catch(const std::exception&){failed=true;}CHECK(failed&&store.savedMessageCount()==101&&store.retentionState().deleted==0&&entities(db));CHECK(store.getSetting("retention_job",dx::Json::object()).empty());CHECK(sqlite3_exec(db,"DROP TRIGGER fail_retention",nullptr,nullptr,nullptr)==SQLITE_OK);
  CHECK(store.retain(50,16,0)==0&&store.savedMessageCount()==101);CHECK(store.retain(50,16,boundary)==16&&store.savedMessageCount()==85);CHECK(entities(db));CHECK(store.retentionState().running);CHECK(store.sessionStats(1501,5,0).messages==store.logRows(5,1000).size());sqlite3_close(db);
 }
 // Resume the persisted job on a new connection; a new arrival cannot join the deletion boundary.
 {dx::Store store(file);auto d=reception(999,"K3ABC");CHECK(store.insert(d,home,country));auto newId=store.lastReceptionId();CHECK(newId>boundary);store.sessionStats(1600,5,0);for(int i=0;i<20&&store.retentionState().running;i++){CHECK(store.retain(50,7,boundary)>=0);store.reclaimPages(4);auto cached=store.sessionStats(1600,5,0);dx::Store reference(file);auto fresh=reference.sessionStats(1600,5,0);CHECK(cached.messages==fresh.messages&&cached.countries==fresh.countries&&cached.stations==fresh.stations&&cached.newestCountries==fresh.newestCountries&&cached.farthest==fresh.farthest);CHECK(cached.latest.size()==fresh.latest.size());for(size_t j=0;j<cached.latest.size();j++)CHECK(cached.latest[j].key()==fresh.latest[j].key());}CHECK(!store.retentionState().running&&store.savedMessageCount()==48);CHECK(store.retentionState().deleted==54);CHECK(store.lastReceptionId()==newId);
  auto records=store.logRows(0,1000);bool found=false;for(const auto& entry:records)if(entry.first==newId)found=true;CHECK(found);auto old=reception(900,"K4ABC","legacy");CHECK(!store.insert(old,home,country));CHECK(store.savedMessageCount()==48);
  sqlite3* db=nullptr;CHECK(sqlite3_open(file.c_str(),&db)==SQLITE_OK);CHECK(entities(db));CHECK(scalar(db,"SELECT count(*) FROM stations AS s WHERE count!=(SELECT count(*) FROM decodes AS d WHERE d.call=s.call AND d.dial=s.dial AND d.mode=s.mode)")==0);
  // Reclassification and timestamp changes update summaries without losing identity references.
  CHECK(sqlite3_exec(db,"UPDATE decodes SET country='Canada',utc=2000,payload=json_set(payload,'$.country','Canada','$.timestamp',2000) WHERE reception_id=(SELECT min(reception_id) FROM decodes)",nullptr,nullptr,nullptr)==SQLITE_OK);CHECK(entities(db));
  CHECK(sqlite3_exec(db,"BEGIN;DELETE FROM decodes WHERE reception_id=(SELECT min(reception_id) FROM decodes);ROLLBACK",nullptr,nullptr,nullptr)==SQLITE_OK);CHECK(entities(db));sqlite3_close(db);
  store.compact();CHECK(store.lastReceptionId()==newId);auto next=reception(2100,"K5ABC");CHECK(store.insert(next,home,country)&&store.lastReceptionId()>newId);CHECK(store.stats(2100).messages==49);
 }
 // Abrupt process exit with an open deletion transaction must leave committed data intact.
 auto childFile=file.string();char* childArgs[]={argv[0],const_cast<char*>("--crash-writer"),childFile.data(),nullptr};pid_t child=-1;
 CHECK(posix_spawn(&child,argv[0],nullptr,nullptr,childArgs,environ)==0);int exit=0;pid_t waited;do{waited=waitpid(child,&exit,0);}while(waited<0&&errno==EINTR);CHECK(waited==child&&WIFEXITED(exit)&&WEXITSTATUS(exit)==0);
 {dx::Store store(file);CHECK(store.savedMessageCount()==49&&store.stats(2100).messages==49);sqlite3* db=nullptr;CHECK(sqlite3_open(file.c_str(),&db)==SQLITE_OK);CHECK(entities(db)&&scalar(db,"SELECT count(*) FROM pragma_integrity_check WHERE integrity_check!='ok'")==0);sqlite3_close(db);}
 // A manual deletion can finish the retention target early; never over-delete afterward.
 {dx::Store store(root/"overlap.sqlite3");for(int i=0;i<30;i++){auto d=reception(3000+i,i%2?"DL1ABC":"K1ABC");CHECK(store.insert(d,home,country));}CHECK(store.retain(20,4)==4);dx::SavedCleanup q;q.kind=dx::CleanupKind::Country;q.country="Germany";store.deleteSaved(q,store.lastReceptionId());auto count=store.savedMessageCount();CHECK(count<18&&store.retain(20,4)==0&&store.savedMessageCount()==count&&!store.retentionState().running);}
 {auto path=root/"cache-expiry.sqlite3";dx::Store store(path);auto far=reception(1,"K1ABC");far.message="CQ K1ABC QF56";CHECK(store.insert(far,home,country));for(int i=0;i<20;i++){auto near=reception(1000+i,"DL1ABC");near.message="CQ DL1ABC KN40";CHECK(store.insert(near,home,country));}CHECK(store.sessionStats(2000,0,1).farthestCall=="K1ABC");auto unconsumed=reception(2,"K2ABC");CHECK(store.insert(unconsumed,home,country));CHECK(store.retain(10,2)==2);auto cached=store.sessionStats(2000,0,1);dx::Store reference(path);auto fresh=reference.sessionStats(2000,0,1);CHECK(cached.messages==20&&cached.messages==fresh.messages&&cached.farthestCall=="DL1ABC"&&cached.farthest==fresh.farthest&&cached.newestCountries==fresh.newestCountries&&cached.stations==1&&cached.countries==1);}
 auto async=root/"async";std::filesystem::create_directory(async);auto current=int64_t(time(nullptr));
 {dx::Store store(root/"filters.sqlite3");for(int i=0;i<3;i++){auto d=reception(4000+i,i==0?"K1ABC":i==1?"DL1ABC":"K2ABC");if(i<2)CHECK(store.insert(d,home,country));else CHECK(store.insert(d,home));}dx::Query all;all.messages=true;auto original=store.rows(all);for(const auto& pattern:{"*","gERM*","Germany, United States","No such country"," , ","*States"})for(bool exclude:{false,true}){auto q=all;if(exclude)q.filter.excludeCountry=pattern;else q.filter.country=pattern;size_t expected=0;for(const auto& row:original)if(q.filter.accepts(row.decode))++expected;CHECK(store.rows(q).size()==expected);int64_t count=0;store.rows(q,0,0,&count);CHECK(count==int64_t(expected));}}
 {dx::Store store(async/"history.sqlite3");for(int i=0;i<10010;i++){auto d=reception(current-20000+i,i%2?"DL1ABC":"K1ABC");CHECK(store.insert(d,home,country));}store.rebaseDistances(home);store.reclassifyCountries(country);store.setSetting("ui",{{"history_all",true},{"history_auto_trim",true},{"history_max_records",10000}});}
 auto mock=async/"dx_mock_valid_small";std::filesystem::create_symlink(std::filesystem::absolute(argv[1]),mock);
 {dx::Engine engine(async,mock,home,country);auto wait=[&](auto predicate){auto end=dx::monotonicMs()+20000;while(dx::monotonicMs()<end){if(predicate(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return predicate(*engine.snapshot());};CHECK(wait([](const auto& s){return s.retention.running;}));
  // Receive a fresh decode while cleanup is active. The CSV must contain all originals,
  // even though SQLite will retain only its recent slice.
  engine.settings({{"history_auto_trim",true},{"history_max_records",10000}});engine.active(true);dx::Job job;job.mode="FT8";job.dial=7074000;job.utcMs=current*1000;job.deadlineMs=dx::monotonicMs()+30000;job.pcm.resize(180000);engine.enqueue(job);dx::Query q;q.messages=true;q.sort=18;engine.query(q,{},21);CHECK(wait([](const auto& s){return s.generation==21&&s.decodedJobs==1&&!s.retention.running&&s.savedMessages==9503;}));CHECK(engine.snapshot()->retention.deleted==510&&engine.snapshot()->tableCount==9503&&engine.snapshot()->error.empty()&&engine.snapshot()->retentionError.empty());CHECK(engine.snapshot()->historyChecks==1&&engine.snapshot()->historyBytes>0);engine.settings({{"history_max_records",20000}});CHECK(wait([](const auto& s){return s.historyChecks==2&&s.retention.limit==20000;}));engine.settings({{"history_trim_to",15000}});CHECK(wait([](const auto& s){return s.historyChecks==3&&s.retention.target==15000;}));
 }
 {dx::Store store(async/"history.sqlite3");dx::CsvLog csv;CHECK(!csv.sync(store,async/"receptions.csv",100*1024*1024));CHECK(csv.cursor()==10013&&store.savedMessageCount()==9503);}
 std::cout<<"Retention rollback, restart, reception boundary, stable IDs, summaries, CSV catch-up and incremental compaction passed\n";return 0;
}
