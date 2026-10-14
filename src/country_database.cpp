// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "country_database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
namespace dx {
namespace {
std::string trim(std::string value){auto a=value.find_first_not_of(" \t\r\n"),b=value.find_last_not_of(" \t\r\n");return a==std::string::npos?"":value.substr(a,b-a+1);}
}
void CountryDatabase::load(const std::filesystem::path& path){
 std::ifstream input(path);if(!input)throw std::runtime_error("Country database is missing from the plugin package");
 dxcc.clear();std::string line;
 while(std::getline(input,line)){
  if(trim(line).empty())continue;
  if(!std::isspace(static_cast<unsigned char>(line.front()))){
   std::vector<std::string> fields;std::istringstream header(line);std::string field;while(std::getline(header,field,':'))fields.push_back(trim(field));
   if(fields.size()<8)throw std::runtime_error("Invalid country database header");
   dxcc.push_back({fields[0],fields[3],{}});continue;
  }
  if(dxcc.empty())throw std::runtime_error("Country prefix before country header");
  std::istringstream aliases(line);std::string alias;
  while(std::getline(aliases,alias,',')){
   alias=trim(alias);if(alias.empty())continue;Prefix prefix;prefix.exact=alias.front()=='=';if(prefix.exact)alias.erase(0,1);
   auto end=alias.find_first_of("([{<~; \t\r\n");prefix.value=alias.substr(0,end);if(!prefix.value.empty())dxcc.back().prefixes.push_back(std::move(prefix));
  }
 }
 if(dxcc.empty())throw std::runtime_error("Empty country database");
}
}
