// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include <algorithm>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto path=std::filesystem::temp_directory_path()/("dx-columns-"+std::to_string(getpid())+".sqlite3");
 std::vector<dx::TableColumnSetting> columns;for(int i=0;i<18;i++)columns.push_back({17-i,85.5f+i*10,i!=8});
 const auto saved=dx::columnSettingsJson(columns);auto decoded=dx::columnSettingsFromJson(saved,18);CHECK(decoded&&dx::columnSettingsJson(*decoded)==saved);
 auto invalid=saved;invalid[0]["order"]=invalid[1]["order"];CHECK(!dx::columnSettingsFromJson(invalid,18));invalid=saved;invalid[0]["order"]=18;CHECK(!dx::columnSettingsFromJson(invalid,18));invalid=saved;invalid[0]["width"]=-1;CHECK(!dx::columnSettingsFromJson(invalid,18));invalid=saved;invalid[0]["visible"]="yes";CHECK(!dx::columnSettingsFromJson(invalid,18));CHECK(!dx::columnSettingsFromJson(dx::Json::array(),18)&&!dx::columnSettingsFromJson(saved,17));
 {dx::Store store(path);store.setSetting("ui",{{"table_columns",saved}});}
 {dx::Store store(path);auto restored=dx::columnSettingsFromJson(store.getSetting("ui",dx::Json::object())["table_columns"],18);CHECK(restored&&dx::columnSettingsJson(*restored)==saved);}
 const int64_t now=1800000000;dx::Query query;query.uniqueCalls=true;query.referenceUtc=now;
 {dx::Store store(path);auto insert=[&](const std::string& message,int age,const std::string& mode,int snr,int audio,std::optional<double> dt){dx::Decode d;d.utc=now-age;d.dial=mode=="FT4"?7047500:7074000;d.mode=mode;d.message=message;d.snr=snr;d.audio=audio;d.dt=dt;d.source="live";if(!store.insert(d,{39.5,33}))throw std::runtime_error("Fixture rejected");};
  insert("CQ K1ABC FN42",150,"FT4",-25,1200,.1);insert("G1ABC K1ABC -08",60,"FT8",-8,1600,-.25);insert("G1ABC K1ABC -08",15,"FT8",-8,1600,-.25);insert("CQ K2ABC FN03",40,"FT4",-20,900,.5);insert("CQ F1ABC",20,"FT8",-12,1000,{});insert("HELLO WORLD",10,"FT8",-15,1000,{});insert("G1ABC <1:123> -10",5,"FT8",-10,1100,{});
  const auto all=store.rows(query);CHECK(all.size()==5);auto sender=std::find_if(all.begin(),all.end(),[](const dx::Row& r){return r.decode.identity.transmitter=="K1ABC";});CHECK(sender!=all.end()&&sender->decode.mode=="FT8"&&dx::targetCall(sender->decode.identity)=="G1ABC"&&sender->count==3&&sender->firstHeardUtc==now-60&&sender->stationFirstHeardUtc==now-150);
  const std::array<double,10> values={-8,*sender->decode.km,7.074,1600,-.25,*sender->decode.bearing,3,15,60,150};
  for(size_t metric=0;metric<values.size();metric++)for(int variant=0;variant<4;variant++){
   auto q=query;auto& range=q.filter.columnRanges[metric];double value=values[metric];if(variant==0)range={value,value};if(variant==1)range.min=value;if(variant==2)range.max=value;if(variant==3)range={value+1,value};
   auto filter=dx::Filter::fromJson(q.filter.json());CHECK(filter.json()==q.filter.json());std::vector<std::string> expected,actual;for(const auto& row:all)if(filter.accepts(row,now))expected.push_back(row.decode.key());for(const auto& row:store.rows(q))actual.push_back(row.decode.key());CHECK(expected==actual);int64_t count=0;store.rows(q,0,0,&count);CHECK(count==int64_t(actual.size()));auto page=store.rows(q,1,1);CHECK(page.size()==(actual.size()>1?1u:0u));if(!page.empty())CHECK(page.front().decode.key()==actual[1]);
  }
  auto q=query;q.filter.call="K1ABC";q.filter.columnRanges[size_t(dx::ColumnMetric::Snr)].max=-20;CHECK(store.rows(q).empty()); // Must not replace latest row with an older matching decode.
  q=query;q.filter.called="G1*";auto rows=store.rows(q);CHECK(rows.size()==2);for(const auto& row:rows)CHECK(q.filter.accepts(row,now));q.filter.excludeCalled="G1ABC";CHECK(store.rows(q).empty());
  q=query;q.filter.mode="FT4";rows=store.rows(q);CHECK(rows.size()==2);for(const auto& row:rows)CHECK(row.decode.mode=="FT4"&&q.filter.accepts(row,now));q.filter.hasTarget=true;CHECK(store.rows(q).empty());
  q=query;q.filter.hasTarget=true;CHECK(store.rows(q).size()==2);CHECK(dx::targetCall(dx::meaning("CQ K1ABC FN42")).empty()&&dx::targetCall(dx::meaning("QRZ K1ABC FN42")).empty()&&dx::targetCall(dx::meaning("DE K1ABC FN42")).empty());
  q=query;q.filter.call="K1ABC";q.filter.columnRanges[size_t(dx::ColumnMetric::MsgAge)].max=15;CHECK(store.rows(q).size()==1);q.referenceUtc++;CHECK(store.rows(q).empty());
  q=query;q.firstSince=now-100;q.filter.call="K1ABC";q.filter.columnRanges[size_t(dx::ColumnMetric::StationFirstAge)]={60,60};CHECK(store.rows(q).size()==1);
  q=query;q.messages=true;q.filter.columnRanges[size_t(dx::ColumnMetric::Count)]={1,1};rows=store.rows(q);CHECK(rows.size()==7);int64_t count=0;store.rows(q,0,0,&count);CHECK(count==7);
  q=query;q.uniqueCalls=false;q.filter.columnRanges[size_t(dx::ColumnMetric::Count)]={2,2};rows=store.rows(q);CHECK(rows.size()==1&&rows.front().decode.identity.transmitter=="K1ABC"&&rows.front().count==2);
  CHECK(!dx::columnRangesFromJson({{"msgAge",{{"min","bad"}}}})[size_t(dx::ColumnMetric::MsgAge)].active());
 }
 std::filesystem::remove(path);std::filesystem::remove(path.string()+"-wal");std::filesystem::remove(path.string()+"-shm");std::cout<<"Column persistence, target/mode filters, numeric and age ranges passed\n";
}
