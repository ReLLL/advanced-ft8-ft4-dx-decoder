// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "country_database.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){
 if(argc!=2)return 2;dx::CountryDatabase database;database.load(argv[1]);CHECK(database.dxcc.size()>300);
 bool us=false,germany=false;for(const auto& country:database.dxcc){
  if(country.name=="United States"){CHECK(country.continent=="NA");for(const auto& prefix:country.prefixes)if(!prefix.exact&&prefix.value=="N")us=true;}
  if(country.name=="Fed. Rep. of Germany")germany=true;
  for(const auto& prefix:country.prefixes)CHECK(!prefix.value.empty()&&prefix.value.find_first_of("([{<~; \t\r\n")==std::string::npos);
 }CHECK(us&&germany);
 auto fixture=std::filesystem::temp_directory_path()/("dx-cty-"+std::to_string(getpid())+".dat");
 {std::ofstream output(fixture);output<<"Test: 1: 2: EU: 40: -30: -3: T:\n    T,=T1EX(2)[3]{EU}<40/-30>~3~,\n    TA;\n";}
 database.load(fixture);CHECK(database.dxcc.size()==1);const auto& prefixes=database.dxcc.front().prefixes;CHECK(prefixes.size()==3&&prefixes[0].value=="T"&&!prefixes[0].exact&&prefixes[1].value=="T1EX"&&prefixes[1].exact&&prefixes[2].value=="TA");
 {std::ofstream output(fixture);output<<"    T;\n";}bool rejected=false;try{database.load(fixture);}catch(const std::exception&){rejected=true;}CHECK(rejected);std::filesystem::remove(fixture);
 std::cout<<"CTY snapshot, continuations, exact calls, overrides and malformed data passed\n";
}
