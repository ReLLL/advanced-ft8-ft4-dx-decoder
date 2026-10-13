// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "csv_log.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
std::string read(const std::filesystem::path& path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
int main(int argc,char** argv){if(argc!=2)return 2;auto folder=std::filesystem::path(argv[1]).parent_path();dx::Store store(folder/"history.sqlite3");dx::Decode d;d.utc=1800000000;d.slotMs=d.utc*1000;d.dial=7074000;d.mode="FT8";d.audio=1200;d.snr=-18;d.dt=.35;d.message="CQ K1ABC FN42";d.country="Country, \"quoted\"";d.raw="first, line\r\nsecond \"line\"";
 CHECK(store.insert(d,{39,33}));auto text=d;text.utc+=15;text.message="HELLO, \"world\"\r\nnext line";CHECK(store.insert(text,{39,33}));auto file=std::filesystem::path(argv[1]);dx::CsvLog log;while(log.sync(store,file,1024*1024)){}CHECK(log.cursor()==2);auto before=read(file);CHECK(!log.sync(store,file,1024*1024)&&read(file)==before);CHECK(!store.insert(text,{39,33}));
 {dx::CsvLog restarted;CHECK(!restarted.sync(store,file,1024*1024));CHECK(restarted.cursor()==2&&read(file)==before);}
 {std::ofstream partial(file,std::ios::app|std::ios::binary);partial<<"3,\"incomplete";}
 {dx::CsvLog recovered;CHECK(!recovered.sync(store,file,1024*1024));CHECK(recovered.cursor()==2&&read(file)==before);}
 auto limited=folder/"limited.csv";dx::CsvLog bounded;auto large=text;large.raw=std::string(4096,'x')+"\r\n\"quoted, end\"";
 for(int n=0;n<500;n++){large.utc+=15;large.message="MESSAGE "+std::to_string(n);CHECK(store.insert(large,{39,33}));}
 while(bounded.sync(store,limited,1024*1024)){}CHECK(bounded.bytes()<=1024*1024);CHECK(bounded.cursor()==502);CHECK(store.stats(d.utc).messages==502);auto tail=read(limited);CHECK(tail.find("MESSAGE 499")!=std::string::npos&&tail.find("MESSAGE 0\"")==std::string::npos);
 {dx::CsvLog restarted;CHECK(!restarted.sync(store,limited,1024*1024));CHECK(restarted.cursor()==502&&read(limited)==tail);}
 auto foreign=folder/"foreign.csv";{std::ofstream output(foreign);output<<"User file,keep it\n";}bool refused=false;try{bounded.sync(store,foreign,1024*1024);}catch(const std::exception&){refused=true;}CHECK(refused&&read(foreign)=="User file,keep it\n");
 auto relocated=folder/"relocated.csv";while(bounded.sync(store,relocated,1024*1024)){}CHECK(bounded.cursor()==502&&read(limited)==tail);
 std::cout<<"CSV append, restart, partial-write recovery, size cap, relocation and foreign-file protection passed\n";return 0;
}
