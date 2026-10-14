// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto folder=std::filesystem::temp_directory_path()/("dx-history-loading-"+std::to_string(getpid()));std::filesystem::create_directory(folder);
 auto file=folder/"history.sqlite3";dx::Point home{39,33};auto timestamp=int64_t(time(nullptr))-86400;
 {dx::Store store(file);for(int i=0;i<250;i++){dx::Decode d;d.utc=timestamp+i;d.slotMs=d.utc*1000;d.dial=7074000;d.mode="FT8";d.snr=-12;d.audio=1000;d.message="CQ K"+std::to_string(i)+"ABC FN42";CHECK(store.insert(d,home,[](auto&){return std::make_pair("United States","NA");}));}store.rebaseDistances(home);store.reclassifyCountries([](auto&){return std::make_pair("United States","NA");});}
 // A retained log with repeats, created in one transaction so fixture setup is cheap.
 sqlite3* db=nullptr;CHECK(sqlite3_open(file.c_str(),&db)==SQLITE_OK);
 auto sql="WITH RECURSIVE repeats(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM repeats WHERE n<99) INSERT INTO decodes SELECT key||':'||n,utc+n*300,dial,mode,call,called,country,continent,grid,type,band,snr,dt,km,message,json_set(payload,'$.timestamp',utc+n*300,'$.slot_ms',(utc+n*300)*1000) FROM decodes CROSS JOIN repeats; UPDATE counters SET value=(SELECT count(*) FROM decodes) WHERE key='messages';";
 CHECK(sqlite3_exec(db,sql,nullptr,nullptr,nullptr)==SQLITE_OK);sqlite3_close(db);
 dx::Engine engine(folder,"/tmp/unused-dx-helper",home,[](auto&){return std::make_pair("United States","NA");});
 auto until=dx::monotonicMs()+10000;while(!engine.snapshot()->generation&&dx::monotonicMs()<until)std::this_thread::sleep_for(std::chrono::milliseconds(10));CHECK(engine.snapshot()->generation);
 engine.active(true);engine.settings({{"history_all",true}});dx::Query table,map;table.sort=17;engine.query(table,map,20);
 auto start=dx::monotonicMs();bool loaded=false;
 while(dx::monotonicMs()-start<8000){
  // Model the live UI's timer refreshing rolling bounds while restoration runs.
  table.referenceUtc=map.referenceUtc=time(nullptr);engine.refresh(table,map);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));auto s=engine.snapshot();if(s->historyAll&&s->tableCount==250&&s->stats.messages==25000){loaded=true;break;}
 }
 CHECK(loaded&&engine.snapshot()->generation==20);std::cout<<"Live history load: "<<dx::monotonicMs()-start<<" ms; query "<<engine.snapshot()->queryMs<<" ms\n";
 engine.settings({{"history_all",false}});engine.query({}, {},30);until=dx::monotonicMs()+5000;while(engine.snapshot()->historyAll&&dx::monotonicMs()<until)std::this_thread::sleep_for(std::chrono::milliseconds(10));CHECK(!engine.snapshot()->historyAll&&engine.snapshot()->table.empty()&&engine.snapshot()->map.empty());
 return 0;
}
