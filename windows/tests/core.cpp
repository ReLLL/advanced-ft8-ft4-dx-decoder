// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "csv_log.h"
#include "fnmatch.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cmath>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(0)
int main(){try{
 CHECK(dx::matches("K1ABC","K?,K1*"));CHECK(dx::matches("K1ABC","K[0-9]A?C"));CHECK(!dx::matches("K1ABC","K[!0-9]*"));
 CHECK(dx::ageText(1700000000,1700000600)=="600s ago");CHECK(dx::ageLabel(1700000000,1700000601)==std::string("Date"));
 CHECK(dx::localDateTimeParse("2024-02-30")==std::nullopt);CHECK(dx::localDateTimeParse("2024-13-01")==std::nullopt);
 auto home=dx::gridPoint("KM69"),station=dx::gridPoint("FN42");CHECK(home&&station);
 auto distance=dx::distanceBearing(*home,*station);CHECK(distance.first>7000&&distance.first<9000&&std::isfinite(distance.second));
 auto folder=std::filesystem::temp_directory_path()/(L"AdvancedDx-test-"+std::to_wstring(GetCurrentProcessId()));CHECK(!std::filesystem::exists(folder));std::filesystem::create_directory(folder);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{folder};
 {
  dx::Store store(folder/L"receptions-\u53d7\u4fe1-\u00e9.sqlite3");dx::Decode reception;reception.utc=1700000000;reception.slotMs=reception.utc*1000;reception.dial=14074000;reception.mode="FT8";reception.message="CQ K1ABC FN42";reception.snr=-12;
  CHECK(store.insert(reception,*home,[](const auto&){return std::make_pair("United States","NA");}));
  dx::Query query;query.messages=true;auto rows=store.rows(query,0,10);CHECK(rows.size()==1&&rows[0].decode.identity.transmitter=="K1ABC");
  dx::CsvLog log;auto file=folder/L"receptions-\u53d7\u4fe1-\u00e9.csv";log.sync(store,file,1024*1024);CHECK(std::filesystem::file_size(file)>dx::csvLogHeader().size());
  CHECK(log.purge(store,file,"K1ABC",INT64_MAX)==1);log.sync(store,file,1024*1024);CHECK(std::filesystem::file_size(file)==dx::csvLogHeader().size());
 }
 std::cout<<"Windows core, Unicode SQLite/CSV paths, cleanup, filters, times and distances passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
