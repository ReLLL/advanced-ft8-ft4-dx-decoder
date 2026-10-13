// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include "helper_client.h"
#include <iostream>
#include <unistd.h>
int main(int argc,char** argv){if(argc!=2)return 2;std::filesystem::path file=argv[1];dx::Store store(file);sqlite3* db=nullptr;sqlite3_open(file.c_str(),&db);
 char* error=nullptr;sqlite3_exec(db,"BEGIN",nullptr,nullptr,&error);sqlite3_stmt* insert=nullptr;
 sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO decodes VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",-1,&insert,nullptr);
 auto begin=dx::monotonicMs();for(int i=0;i<1000000;i++){dx::Decode d;d.utc=1800000000+i;d.dial=7074000;d.mode="FT8";d.message="CQ K"+std::to_string(i%1000)+"ABC FN42";d.identity=dx::meaning(d.message);d.country="Country "+std::to_string(i%10);d.snr=-10;d.audio=1000;d.source="benchmark";auto payload=d.json().dump();auto key=std::to_string(i);
  sqlite3_reset(insert);auto text=[&](int n,const std::string& v){sqlite3_bind_text(insert,n,v.c_str(),int(v.size()),SQLITE_TRANSIENT);};text(1,key);sqlite3_bind_int64(insert,2,d.utc);sqlite3_bind_int64(insert,3,d.dial);text(4,d.mode);text(5,d.identity.transmitter);text(6,"CQ");text(7,d.country);text(8,"NA");text(9,"FN42");text(10,"CQ");text(11,"40m");sqlite3_bind_int(insert,12,d.snr);sqlite3_bind_double(insert,13,.2);sqlite3_bind_double(insert,14,8000);text(15,d.message);text(16,payload);if(sqlite3_step(insert)!=SQLITE_DONE){std::cerr<<sqlite3_errmsg(db);return 1;}}
 sqlite3_finalize(insert);sqlite3_exec(db,"INSERT OR REPLACE INTO stations SELECT call,dial,mode,max(utc),count(*),payload FROM decodes GROUP BY call,dial,mode;COMMIT",nullptr,nullptr,&error);if(error){std::cerr<<error;sqlite3_free(error);return 1;}sqlite3_close(db);
 dx::Json result={{"rows",1000000},{"seed_seconds",(dx::monotonicMs()-begin)/1000.}};
 dx::Query q;begin=dx::monotonicMs();auto all=store.rows(q);result["all_history_ms"]=dx::monotonicMs()-begin;result["stations"]=all.size();q.filter.country="Country 2";begin=dx::monotonicMs();auto filtered=store.rows(q);result["country_filter_ms"]=dx::monotonicMs()-begin;result["filtered_stations"]=filtered.size();q.filter={};q.filter.search="K12";begin=dx::monotonicMs();auto search=store.rows(q);result["search_ms"]=dx::monotonicMs()-begin;result["search_stations"]=search.size();std::cout<<result.dump(2)<<'\n';return result["country_filter_ms"].get<int>()<1000&&result["search_ms"].get<int>()<1000?0:1;
}
