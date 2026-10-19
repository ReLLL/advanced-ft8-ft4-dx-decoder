// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <regex>
#include <map>
#include <optional>
// The shared engine uses fnmatch with flags=0 only. Cache compiled patterns on
// each worker, including invalid patterns, instead of recompiling for every row.
inline int fnmatch(const char* pattern,const char* value,int flags){
 if(flags)return 1;
 thread_local std::map<std::string,std::optional<std::regex>> cache;
 auto found=cache.find(pattern);
 if(found==cache.end()){
  if(cache.size()>=128)cache.clear();std::string expression;
  bool bracket=false;
  for(size_t i=0;pattern[i];i++){
   char c=pattern[i];
   if(c=='\\'&&pattern[i+1]){char literal=pattern[++i];if(std::string(".^$|(){}+\\[]*?").find(literal)!=std::string::npos)expression+='\\';expression+=literal;continue;}
   if(c=='['){bracket=true;expression+='[';if(pattern[i+1]=='!'){expression+='^';++i;}continue;}
   if(c==']'&&bracket){bracket=false;expression+=c;continue;}
   if(bracket){expression+=c;continue;}
   if(c=='*')expression+=".*";else if(c=='?')expression+='.';else{if(std::string(".^$|(){}+\\").find(c)!=std::string::npos)expression+='\\';expression+=c;}
  }
  std::optional<std::regex> compiled;
  try{compiled.emplace(expression,std::regex::ECMAScript|std::regex::optimize);}catch(const std::regex_error&){}
  found=cache.emplace(pattern,std::move(compiled)).first;
 }
 return found->second&&std::regex_match(value,*found->second)?0:1;
}
