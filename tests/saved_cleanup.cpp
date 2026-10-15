// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
namespace {
std::string read(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
template<class Condition> bool wait(dx::Engine& e,Condition condition){auto until=dx::monotonicMs()+10000;while(dx::monotonicMs()<until){if(condition(*e.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return condition(*e.snapshot());}
dx::Decode reception(int64_t utc,const std::string& message){dx::Decode d;d.utc=utc;d.slotMs=utc*1000;d.dial=7074000;d.mode="FT8";d.snr=-12;d.audio=1000;d.message=message;d.raw="comma, \"quotes\"\r\nsecond line";return d;}
}
int main(){
 auto start=dx::cleanupDate("2026-10-10"),end=dx::cleanupDate("2026-10-10",true);CHECK(start&&end&&*end-*start==86399);CHECK(!dx::cleanupDate("2026-02-30")&&!dx::cleanupDate("2026-10-10 bad")&&!dx::cleanupDate("2026-10-10 25:00:00"));CHECK(dx::cleanupDate("2024-02-29"));
 auto folder=std::filesystem::temp_directory_path()/("dx-saved-cleanup-"+std::to_string(getpid()));std::filesystem::create_directory(folder);TestFolderCleanup cleanup(folder);dx::Point home{39,33};auto country=[](const std::string& call){return std::make_pair(call.rfind("K",0)==0?"United States":"Germany","NA");};
 for(int kind=0;kind<=6;kind++){
  auto root=folder/std::to_string(kind);std::filesystem::create_directory(root);dx::Store store(root/"history.sqlite3");dx::CsvLog csv;
  for(auto d:{reception(*start-1,"CQ K1ABC FN42"),reception(*start,"CQ DL1ABC JO62"),reception(*start+1,"K1ABC DL1ABC -12"),reception(*end,"CQ K2ABC"),reception(*end+1,"CQ DL2ABC")})CHECK(store.insert(d,home,country));
  while(csv.sync(store,root/"first.csv",1024*1024)){}while(csv.sync(store,root/"second.csv",1024*1024)){}
  dx::SavedCleanup criteria;criteria.kind=dx::CleanupKind(kind);criteria.from=*start;criteria.to=*end;criteria.country="United States";auto through=store.lastReceptionId();auto records=store.logRows(0);int64_t expected=0;std::string kept=dx::csvLogHeader();for(const auto& entry:records){if(criteria.matches(entry.second))++expected;else kept+=dx::csvLogRow(entry.first,entry.second);}CHECK(store.countCleanup(criteria,through)==expected);
  // A same-criteria reception added after the preview is outside this one-off operation.
  auto future=reception(*start-2,"CQ K3ABC FN42");CHECK(store.insert(future,home,country));auto futureId=store.lastReceptionId();while(csv.sync(store,root/"first.csv",1024*1024)){}while(csv.sync(store,root/"second.csv",1024*1024)){}kept+=dx::csvLogRow(futureId,future);
  CHECK(store.deleteSaved(criteria,through)==expected);store.compact();CHECK(store.lastReceptionId()==futureId);for(const auto& entry:store.logRows(0))if(entry.first<=through)CHECK(!criteria.matches(entry.second));CHECK(store.ignoredCalls().empty()&&store.ignoredMessages().empty());
  for(const auto& name:{"first.csv","second.csv"}){CHECK(csv.purgeSaved(store,root/name,criteria,through)==expected);CHECK(read(root/name)==kept);CHECK(!csv.sync(store,root/name,1024*1024));}
  CHECK(store.savedMessageCount()==6-expected);CHECK(store.getSetting("cleanup_jobs",dx::Json::object()).size()==1);
 }
 // Missing numeric distance and cached distance are separate, explicit choices.
 {dx::Store store(folder/"quality.sqlite3");auto a=reception(*start,"CQ K1ABC FN42"),b=reception(*start+1,"DL1ABC K1ABC -12");CHECK(store.insert(a,home,country)&&store.insert(b,home,country));dx::SavedCleanup q;q.kind=dx::CleanupKind::MissingDistance;CHECK(store.countCleanup(q,store.lastReceptionId())==0);q.includeCached=true;CHECK(store.countCleanup(q,store.lastReceptionId())==1);}
 // Migrate the old implicit row IDs before compaction, keeping gaps and CSV IDs.
 {auto file=folder/"old-schema.sqlite3";sqlite3* db=nullptr;CHECK(sqlite3_open(file.c_str(),&db)==SQLITE_OK);CHECK(sqlite3_exec(db,"CREATE TABLE decodes(key TEXT PRIMARY KEY,utc INTEGER NOT NULL,dial INTEGER NOT NULL,mode TEXT NOT NULL,call TEXT NOT NULL,called TEXT,country TEXT,continent TEXT,grid TEXT,type TEXT,band TEXT,snr INTEGER,dt REAL,km REAL,message TEXT,payload TEXT NOT NULL)",nullptr,nullptr,nullptr)==SQLITE_OK);
  auto a=reception(*start,"CQ K1ABC FN42"),b=reception(*start+1,"CQ DL1ABC JO62"),c=reception(*start+2,"CQ K2ABC FN42");sqlite3_stmt* insert=nullptr;CHECK(sqlite3_prepare_v2(db,"INSERT INTO decodes(rowid,key,utc,dial,mode,call,message,payload) VALUES(?,?,?,?,?,?,?,?)",-1,&insert,nullptr)==SQLITE_OK);int index=0;
  for(auto d:{a,b,c}){d.identity=dx::meaning(d.message);sqlite3_bind_int64(insert,1,index==0?1:index==1?4:20);sqlite3_bind_int64(insert,3,d.utc);sqlite3_bind_int64(insert,4,d.dial);auto bind=[&](int column,const std::string& value){sqlite3_bind_text(insert,column,value.c_str(),int(value.size()),SQLITE_TRANSIENT);};bind(2,d.key());bind(5,d.mode);bind(6,d.identity.transmitter);bind(7,d.message);bind(8,d.json().dump());CHECK(sqlite3_step(insert)==SQLITE_DONE);sqlite3_reset(insert);sqlite3_clear_bindings(insert);index++;}sqlite3_finalize(insert);sqlite3_close(db);
  dx::Store store(file);auto migrated=store.logRows(0);CHECK(migrated.size()==3&&migrated[0].first==1&&migrated[1].first==4&&migrated[2].first==20);CHECK(store.deleteTransmitter("DL1ABC")==1);auto before=store.logRows(0);store.compact();auto after=store.logRows(0);CHECK(before.size()==2&&after.size()==2&&before[0].first==after[0].first&&before[1].first==after[1].first&&after[1].first==20);}
 // Failed CSV cleanup survives restart; newer receptions and IDs remain intact.
 auto root=folder/"async";std::filesystem::create_directory(root);
 {dx::Store store(root/"history.sqlite3");for(auto d:{reception(*start-1,"CQ K1ABC FN42"),reception(*start+1,"CQ DL1ABC JO62")})CHECK(store.insert(d,home,country));dx::CsvLog csv;while(csv.sync(store,root/"receptions.csv",1024*1024)){}std::ofstream(root/"blocked.csv")<<"unrelated file\n";store.setSetting("csv_paths",dx::Json::array({(root/"receptions.csv").string(),(root/"blocked.csv").string()}));store.setSetting("ui",{{"history_all",true}});}
 {dx::Engine e(root,"/tmp/unused-dx-helper",home,country);CHECK(wait(e,[](const auto& s){return s.historyAll&&s.tableCount==2;}));dx::SavedCleanup q;q.kind=dx::CleanupKind::Before;q.from=*start;auto id=e.previewCleanup(q);CHECK(wait(e,[&](const auto& s){return s.cleanupPreviewId==id&&s.cleanupMatches==1;}));auto through=e.snapshot()->cleanupThroughId;auto deleted=e.deleteSaved(q,through);CHECK(wait(e,[&](const auto& s){return s.completedCleanup>=deleted&&s.cleanupBusy&&!s.cleanupError.empty()&&s.stats.messages==1&&s.tableCount==1&&s.mapCount==1;}));CHECK(read(root/"blocked.csv")=="unrelated file\n");}
 {dx::Store store(root/"history.sqlite3");auto d=reception(*start-1,"CQ K1ABC FN42");d.identity=dx::meaning(d.message);std::ofstream(root/"blocked.csv",std::ios::binary)<<dx::csvLogHeader()<<dx::csvLogRow(1,d);}
 {dx::Engine e(root,"/tmp/unused-dx-helper",home,country);CHECK(wait(e,[](const auto& s){return s.generation&&!s.cleanupBusy&&s.cleanupError.empty()&&s.stats.messages==1;}));CHECK(read(root/"blocked.csv")==dx::csvLogHeader());dx::Store store(root/"history.sqlite3");CHECK(store.getSetting("cleanup_jobs",dx::Json::object()).empty());}
 std::filesystem::remove_all(folder);std::cout<<"Date boundaries, independent cleanups, CSV bytes, future reception retention, stable IDs and restart retry passed\n";return 0;
}
