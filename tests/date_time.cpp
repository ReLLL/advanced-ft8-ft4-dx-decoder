// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include "saved_cleanup.h"
#include "csv_log.h"
#include "temporary_folder.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
void zone(const char* name){setenv("TZ",name,1);tzset();}
int main(){
 const int64_t midnight=1704067200;zone("Europe/Istanbul");
 CHECK(dx::durationText(0)=="00d 00:00"&&dx::durationText(-1)=="00d 00:00"&&dx::durationText(5*86400+12*3600+40*60+59)=="05d 12:40");
 CHECK(dx::historySpanText(true,midnight,midnight+3661)=="00d 01:01");
 CHECK(dx::historySpanText(false,midnight,midnight+3661).empty()&&dx::historySpanText(true,0,midnight).empty());
 CHECK(dx::historySpanText(true,midnight,midnight-1)=="00d 00:00");
 CHECK(dx::localDateTime(midnight)=="2024-01-01 03:00:00 +03:00");
 CHECK(dx::ageText(midnight,midnight+600)=="600s ago"&&dx::ageText(midnight,midnight+601)==dx::localDateTime(midnight));
 CHECK(dx::localDateTimeParse("2024-01-01 03:00:00")==midnight);
 CHECK(dx::cleanupDate("2024-01-01")==midnight-10800&&dx::cleanupDate("2024-01-01",true)==midnight-10800+86399);
 CHECK(!dx::localDateTimeParse("2024-02-30")&&!dx::localDateTimeParse("2024-01-01 25:00:00")&&!dx::localDateTimeParse("bad"));
 dx::Json old={{"custom_since","2024-01-01 00:00:00"}};
 CHECK(dx::restoredLocalDateTime(old,"custom_since")=="2024-01-01 03:00:00");
 dx::Json saved={{"date_time_style",1},{"custom_since","2024-01-01 03:00:00"},{"custom_since_epoch",midnight}};
 zone("Asia/Kathmandu");CHECK(dx::localDateTime(midnight)=="2024-01-01 05:45:00 +05:45");CHECK(dx::restoredLocalDateTime(saved,"custom_since")=="2024-01-01 05:45:00");
 zone("America/New_York");CHECK(dx::localDateTime(midnight)=="2023-12-31 19:00:00 -05:00");
 dx::Json repeated={{"date_time_style",1},{"custom_since","2024-11-03 01:30:00"},{"custom_since_epoch",1730615400LL}};
 CHECK(dx::restoredDateTimeEpoch(repeated,"custom_since")==1730615400LL&&dx::restoredLocalDateTime(repeated,"custom_since")=="2024-11-03 01:30:00");
 CHECK(!dx::cleanupDate("2024-03-10 02:30:00")&&!dx::cleanupDate("2024-11-03 01:30:00"));
 auto spring=dx::cleanupDate("2024-03-10"),springEnd=dx::cleanupDate("2024-03-10",true),fall=dx::cleanupDate("2024-11-03"),fallEnd=dx::cleanupDate("2024-11-03",true);
 CHECK(spring&&springEnd&&*springEnd-*spring+1==23*3600);CHECK(fall&&fallEnd&&*fallEnd-*fall+1==25*3600);
 CHECK(dx::localDateTime(*spring)=="2024-03-10 00:00:00 -05:00"&&dx::localDateTime(*springEnd)=="2024-03-10 23:59:59 -04:00");
 dx::SavedCleanup q;q.kind=dx::CleanupKind::Between;q.from=*spring;q.to=*springEnd;
 dx::Decode d;for(auto utc:{*spring-1,*spring,*springEnd,*springEnd+1}){d.utc=utc;CHECK(q.matches(d)==(utc>=*spring&&utc<=*springEnd));}
 CHECK(q.description().find("2024-03-10 00:00:00 -05:00")!=std::string::npos&&q.description().find("2024-03-10 23:59:59 -04:00")!=std::string::npos);
 // Identical wall-clock times across a fall-back still sort by their original epochs.
 {dx::Store store(":memory:");int i=0;for(int64_t utc:{1730611800LL,1730615400LL}){dx::Decode entry;entry.utc=utc;entry.dial=7074000;entry.mode="FT8";entry.message="CQ K"+std::to_string(++i)+"ABC FN42";entry.country="United States";CHECK(store.insert(entry,{39,33}));CHECK(dx::csvLogRow(i,entry).find(i==1?"2024-11-03T05:30:00Z":"2024-11-03T06:30:00Z")!=std::string::npos);}
  dx::Query sort;sort.messages=true;sort.sort=18;sort.descending=true;auto rows=store.rows(sort);CHECK(rows.size()==2&&rows[0].decode.utc>rows[1].decode.utc);
  CHECK(dx::localDateTime(rows[0].decode.utc)=="2024-11-03 01:30:00 -05:00"&&dx::localDateTime(rows[1].decode.utc)=="2024-11-03 01:30:00 -04:00");
  auto folder=std::filesystem::temp_directory_path()/("dx-local-time-"+std::to_string(getpid()));CHECK(std::filesystem::create_directory(folder));TestFolderCleanup cleanup(folder);
  auto exported=folder/"export.csv";store.exportRows(sort,exported,true);auto read=[&](){std::ifstream in(exported);return std::string(std::istreambuf_iterator<char>(in),{});};auto messages=read();
  CHECK(messages.substr(0,messages.find('\n')).find("LocalDateTime")!=std::string::npos&&messages.find("2024-11-03 01:30:00 -05:00")!=std::string::npos&&messages.find("2024-11-03 01:30:00 -04:00")!=std::string::npos);
  sort.countries=true;store.exportRows(sort,exported,true);auto countries=read();CHECK(countries.substr(0,countries.find('\n')).find("LocatedSenders,LocalDateTime")!=std::string::npos&&countries.find("2024-11-03 01:30:00 -05:00")!=std::string::npos);
 }
 zone("UTC");CHECK(dx::localDateTime(midnight)=="2024-01-01 00:00:00 +00:00");CHECK(dx::localDateTime(0)=="--");
 CHECK(dx::restoredDateTimeEpoch(repeated,"custom_since")==1730615400LL&&dx::restoredLocalDateTime(repeated,"custom_since")=="2024-11-03 06:30:00");
 std::cout<<"Local dates, offsets, DST boundaries, cleanup cutoffs, saved-range migration and timestamp sorting passed\n";
}
