// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
namespace {
std::string read(const std::filesystem::path& path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
void write(const std::filesystem::path& path,const std::string& text){std::ofstream file(path,std::ios::binary);file<<text;}
dx::Decode reception(int64_t utc,const std::string& message){dx::Decode d;d.utc=utc;d.slotMs=utc*1000;d.dial=7074000;d.mode="FT8";d.audio=1000;d.snr=-10;d.message=message;return d;}
template<class Condition> bool waitFor(dx::Engine& engine,Condition condition){auto start=dx::monotonicMs();while(dx::monotonicMs()-start<6000){if(condition(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return condition(*engine.snapshot());}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;auto folder=std::filesystem::temp_directory_path()/("advanced-dx-moderation-"+std::to_string(getpid()));std::filesystem::create_directory(folder);TestFolderCleanup cleanup(folder);dx::Point home{39,33};int64_t now=time(nullptr);
 auto country=[](const std::string& call){return std::make_pair(call=="K1ABC"?"United States":"Germany","EU");};
 auto file=folder/"history.sqlite3";auto csvPath=folder/"receptions.csv";
 {
  dx::Store store(file);auto a=reception(now,"CQ K1ABC FN42");auto b=reception(now,"CQ DL1ABC JO62");CHECK(store.insert(a,home,country)&&store.insert(b,home,country));
  auto c=a;c.mode="FT4";c.dial=14080000;c.utc++;c.message="DL1ABC K1ABC -12";CHECK(store.insert(c,home,country));
  // Same-slot messages must replace text in both cached and session/grouped rows.
  auto newer=reception(now+15,"DL2ABC DL1ABC -08");CHECK(store.insert(newer,home,country));auto tied=newer;tied.message="DL2ABC DL1ABC R-08";CHECK(store.insert(tied,home,country));
  auto old=newer;old.utc=now+5;old.message="DL2ABC DL1ABC -15";CHECK(store.insert(old,home,country));
  for(int variant=0;variant<4;variant++){dx::Query q;if(variant==1)q.since=now;if(variant==2)q.filter.call="DL1ABC";if(variant==3){q.uniqueCalls=true;q.afterRowId=1;}auto rows=store.rows(q);auto row=std::find_if(rows.begin(),rows.end(),[](const dx::Row& r){return r.decode.identity.transmitter=="DL1ABC";});CHECK(row!=rows.end()&&row->decode.message==tied.message&&row->decode.snr==tied.snr&&row->firstHeardUtc==tied.utc&&row->stationFirstHeardUtc==now);}
  dx::Query messages;messages.messages=true;CHECK(store.rows(messages).size()==6);dx::CsvLog csv;while(csv.sync(store,csvPath,1024*1024)){}auto relocated=folder/"relocated.csv";while(csv.sync(store,relocated,1024*1024)){}
  auto keep=reception(now+30,"CQ DL2ABC JO62");keep.raw="comma, \"quotes\"\r\nmultiple lines";CHECK(store.insert(keep,home,country));while(csv.sync(store,csvPath,1024*1024)){}while(csv.sync(store,relocated,1024*1024)){}
  auto records=store.logRows(0);std::string expected=dx::csvLogHeader();for(const auto& r:records)if(r.second.identity.transmitter!="K1ABC")expected+=dx::csvLogRow(r.first,r.second);
  auto before=store.stats(now+30);CHECK(before.messages==7&&before.stations==3&&before.countries==2&&before.farthestCall=="K1ABC");auto high=store.lastReceptionId();CHECK(store.ignoreTransmitter("k1abc",true)==2&&store.ignored("K1ABC")&&store.lastReceptionId()==high);
  CHECK(csv.purge(store,csvPath,"K1ABC",high)==2&&csv.purge(store,relocated,"K1ABC",high)==2);CHECK(read(csvPath)==expected&&read(relocated)==expected);CHECK(!csv.sync(store,relocated,1024*1024));
  auto stats=store.stats(now+30);CHECK(stats.messages==5&&stats.stations==2&&stats.countries==1&&stats.farthestCall!="K1ABC"&&stats.farthest<before.farthest);CHECK(store.sessionStats(now+30,0,now).messages==5&&store.history("K1ABC",7074000,"FT8").empty());
  a.utc+=60;a.source="legacy";CHECK(!store.insert(a,home,country));CHECK(store.getSetting("csv_paths",dx::Json::array()).size()==2);
  // Tail deletion keeps the reception ID above all prior entries and CSV floors.
  CHECK(store.ignoreTransmitter("DL2ABC",true)==1);CHECK(csv.purge(store,csvPath,"DL2ABC",high)==1);CHECK(!csv.sync(store,csvPath,1024*1024));CHECK(csv.cursor()==high);store.unignoreAll();a.utc+=15;CHECK(store.insert(a,home,country)&&store.lastReceptionId()==high+1);CHECK(csv.sync(store,csvPath,1024*1024));
  CHECK(store.ignoreTransmitter("K1ABC",true)==1);CHECK(store.ignoreTransmitter("DL1ABC",true)==4&&store.rows(dx::Query{}).empty()&&store.stats(now+90).messages==0&&store.stats(now+90).farthest==0);
 }
 {dx::Store store(file);CHECK(store.ignored("DL1ABC"));auto d=reception(now+120,"CQ DL1ABC JO62");CHECK(!store.insert(d,home,country));auto high=store.lastReceptionId();store.unignoreAll();CHECK(store.insert(d,home,country)&&store.lastReceptionId()==high+1);
  auto newHome=dx::Point{0,179};auto expected=dx::distanceBearing(newHome,*d.position);store.rebaseDistances(newHome);dx::Query q;CHECK(std::abs(*store.rows(q).front().decode.km-expected.first)<1e-6&&std::abs(store.stats(now+120).farthest-expected.first)<1e-6);q.since=now;CHECK(std::abs(*store.rows(q).front().decode.bearing-expected.second)<1e-6);CHECK(store.lastReceptionId()==high+1);store.rebaseDistances(home);CHECK(std::abs(*store.rows(q).front().decode.km-*d.km)<1e-6);
 }
 {
  dx::Store store(folder/"edge.sqlite3");dx::CsvLog csv;auto a=reception(now,"CQ DL1ABC JO62"),b=reception(now+15,"CQ K1ABC FN42");CHECK(store.insert(a,home,country)&&store.insert(b,home,country));
  // Simulate a trimmed log containing only the transmitter being removed.
  auto trimmed=folder/"trimmed.csv";write(trimmed,dx::csvLogHeader()+dx::csvLogRow(2,b));CHECK(store.ignoreTransmitter("K1ABC",true)==1);CHECK(csv.purge(store,trimmed,"K1ABC",2)==1);CHECK(read(trimmed)==dx::csvLogHeader());CHECK(!csv.sync(store,trimmed,1024*1024)&&read(trimmed)==dx::csvLogHeader());
  // Receptions after unignore are newer than a pending purge's cutoff.
  store.unignoreAll();b.utc+=30;CHECK(store.insert(b,home,country));CHECK(csv.sync(store,trimmed,1024*1024));auto before=read(trimmed);CHECK(csv.purge(store,trimmed,"K1ABC",2)==0&&read(trimmed)==before);
  auto foreign=folder/"foreign.csv";write(foreign,"unrelated,user,file\n");bool refused=false;try{csv.purge(store,foreign,"K1ABC",3);}catch(const std::exception&){refused=true;}CHECK(refused&&read(foreign)=="unrelated,user,file\n");
  auto partial=folder/"partial.csv";write(partial,dx::csvLogHeader()+dx::csvLogRow(3,b)+"4,\"partial");before=read(partial);refused=false;try{csv.purge(store,partial,"K1ABC",3);}catch(const std::exception&){refused=true;}CHECK(refused&&read(partial)==before);
  auto symlink=folder/"link.csv";std::filesystem::create_symlink(trimmed,symlink);refused=false;try{csv.purge(store,symlink,"K1ABC",3);}catch(const std::exception&){refused=true;}CHECK(refused&&read(trimmed)!=dx::csvLogHeader());CHECK(csv.purge(store,folder/"missing.csv","K1ABC",3)==0);
 }
 {
  dx::Store store(folder/"unresolved.sqlite3");dx::CsvLog csv;auto log=folder/"unresolved.csv";
  for(const auto& token:std::vector<std::string>{"<1:123>","<BROKEN>","<BAD\"TAG>"}){
   auto a=reception(now,"CQ "+token+" FN42");auto b=a;b.mode="FT4";b.dial=14080000;b.utc+=15;CHECK(store.insert(a,home)&&store.insert(b,home));CHECK(a.identity.transmitter==token&&dx::callsignLookupUrl(token,0).empty());while(csv.sync(store,log,1024*1024)){}
   bool accepted=false;int64_t removed=0;try{removed=store.ignoreTransmitter(token,true);accepted=true;}catch(const std::exception&){}CHECK(accepted&&removed==2);CHECK(store.ignored(token)&&store.rows(dx::Query{}).empty()&&store.stats(now+15).messages==0);CHECK(csv.purge(store,log,token,store.lastReceptionId())==2&&read(log)==dx::csvLogHeader());a.utc+=30;CHECK(!store.insert(a,home));
  }
 }
 {
  dx::Store store(folder/"no-identity.sqlite3");dx::CsvLog csv;auto log=folder/"no-identity.csv";
  auto a=reception(now,"HELLO, \"WORLD\"\nNEXT"),b=reception(now+15,"OTHER UNKNOWN TEXT");auto again=a;again.dial=14080000;again.mode="FT4";again.utc++;
  CHECK(store.insert(a,home)&&store.insert(again,home)&&store.insert(b,home));CHECK(a.identity.transmitter.empty()&&b.identity.transmitter.empty());auto target=dx::ignoreTarget(a);CHECK(target&&target->message&&target->matches(again)&&!target->matches(b));while(csv.sync(store,log,1024*1024)){}
  auto records=store.logRows(0);std::string expected=dx::csvLogHeader();for(const auto& entry:records)if(!target->matches(entry.second))expected+=dx::csvLogRow(entry.first,entry.second);
  dx::Query unique;unique.uniqueCalls=true;CHECK(store.ignoreMessage(a.message,true)==2&&store.ignoredMessage(a.message));CHECK(store.rows(unique).size()==1&&store.rows(unique).front().decode.message==b.message);CHECK(csv.purge(store,log,*target,store.lastReceptionId())==2&&read(log)==expected);a.utc+=30;CHECK(!store.insert(a,home));b.utc+=30;CHECK(store.insert(b,home));CHECK(store.rows(unique).size()==1&&store.rows(unique).front().count==2);
  bool emptyRejected=false;try{store.ignoreTransmitter("",true);}catch(const std::exception&){emptyRejected=true;}CHECK(emptyRejected&&store.stats(now+60).messages==2);dx::Decode empty;CHECK(!dx::ignoreTarget(empty));CHECK(store.ignoreTransmitter("not-a-call",true)==0&&store.ignored("NOT-A-CALL"));
  store.unignoreAll();CHECK(!store.ignoredMessage(a.message));CHECK(store.insert(a,home));
 }
 {
  dx::Store store(folder/"hash-ignore.sqlite3");auto token="<1:"+std::to_string(dx::callHash("K1ABC",12))+">";auto hashed=reception(now,"CQ "+token+" FN42");CHECK(store.insert(hashed,home)&&hashed.identity.transmitter==token);CHECK(store.ignoreTransmitter(token,true)==1);auto literal=reception(now+15,"CQ K1ABC FN42");CHECK(store.insert(literal,home));hashed.utc+=30;CHECK(!store.insert(hashed,home));CHECK(store.rows(dx::Query{}).size()==1&&store.rows(dx::Query{}).front().decode.identity.transmitter=="K1ABC");
 }
 auto mock=folder/"dx_mock_valid_small";std::filesystem::create_symlink(std::filesystem::absolute(argv[1]),mock);auto root=folder/"async";
 {
  dx::Engine engine(folder/"separate-actions",mock,home,country);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.generation>0;}));engine.active(true);dx::Job job;job.mode="FT8";job.dial=7074000;job.utcMs=int64_t(time(nullptr))*1000;job.pcm.resize(180000);
  auto receive=[&]{job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);};receive();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==3&&!s.csvCatchingUp&&s.pendingWrites==0;}));
  // Ignore alone preserves all saved views/logs, but rejects later receptions.
  engine.ignoreTransmitter("K0ABC");CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredCalls.size()==1&&!s.moderating&&s.stats.messages==3&&s.tableCount==3;}));CHECK(read(folder/"separate-actions/receptions.csv").find("\"K0ABC\"")!=std::string::npos);
  job.utcMs+=15000;receive();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==5&&!s.csvCatchingUp&&s.pendingWrites==0;}));
  // Delete alone removes history/logs and allows that sender to return.
  auto deleted=engine.moderate({"K1ABC",false},true,false);for(const auto& row:engine.snapshot()->table)CHECK(row.decode.identity.transmitter!="K1ABC");CHECK(waitFor(engine,[&](const dx::Snapshot& s){return s.completedModeration>=deleted&&!s.moderating&&s.stats.messages==3&&s.tableCount==2&&s.ignoredCalls.size()==1;}));CHECK(read(folder/"separate-actions/receptions.csv").find("\"K1ABC\"")==std::string::npos);
  job.utcMs+=15000;receive();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==5&&s.tableCount==3&&!s.csvCatchingUp&&s.pendingWrites==0;}));CHECK(read(folder/"separate-actions/receptions.csv").find("\"K1ABC\"")!=std::string::npos);
  engine.ignoreTransmitter("K2ABC",true);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredCalls.size()==2&&!s.moderating&&s.stats.messages==2&&s.tableCount==2;}));
  auto combined=engine.moderate({"K1ABC",false},true,true);CHECK(waitFor(engine,[&](const dx::Snapshot& s){return s.completedModeration>=combined&&s.ignoredCalls.size()==3&&!s.moderating&&s.stats.messages==1&&s.tableCount==1;}));
  CHECK(engine.snapshot()->table.front().decode.identity.transmitter=="K0ABC");job.utcMs+=15000;receive();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.decodedJobs==4&&s.pendingWrites==0;}));CHECK(engine.snapshot()->stats.messages==1);
 }
 {
  dx::Engine engine(root,mock,home,country);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.generation>0;}));engine.active(true);dx::Job job;job.id=1;job.mode="FT8";job.dial=7074000;job.utcMs=int64_t(time(nullptr))*1000;job.pcm.resize(180000);job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);
  CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==3&&!s.csvCatchingUp&&s.pendingWrites==0;}));auto uptime=engine.snapshot()->stats.activeSeconds;engine.settings({{"csv_path",(root/"other.csv").string()}});CHECK(waitFor(engine,[&](const dx::Snapshot& s){return s.csvPath==(root/"other.csv").string()&&!s.csvCatchingUp;}));
  auto homeStart=dx::monotonicMs();engine.home({0,179});CHECK(dx::monotonicMs()-homeStart<100);auto rebased=dx::distanceBearing({0,179},*dx::gridPoint("FN42")).first;CHECK(waitFor(engine,[&](const dx::Snapshot& s){return std::abs(s.stats.farthest-rebased)<1e-6&&!s.table.empty()&&s.table.front().decode.km&&std::abs(*s.table.front().decode.km-rebased)<1e-6;}));engine.home(home);CHECK(waitFor(engine,[&](const dx::Snapshot& s){return std::abs(s.stats.farthest-dx::distanceBearing(home,*dx::gridPoint("FN42")).first)<1e-6;}));
  engine.settings({{"history_all",true}});CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.tableCount==3&&s.mapCount==3;}));
  // History viewing must not pause decoding, either view, or durable logging.
  job.id++;job.mode="FT4";job.dial=14080000;job.utcMs+=15000;job.pcm.resize(90000);job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);
  CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.stats.messages==6&&s.tableCount==3&&s.mapCount==6&&!s.csvCatchingUp&&s.pendingWrites==0;}));
  auto persisted=read(root/"other.csv");CHECK(persisted.find(",\"FT4\",\"20m\",14080000,")!=std::string::npos);{dx::Store saved(root/"history.sqlite3");CHECK(saved.stats(job.utcMs/1000).messages==6);}
  for(const auto& rows:{engine.snapshot()->table,engine.snapshot()->map})CHECK(std::count_if(rows.begin(),rows.end(),[&](const dx::Row& row){return row.decode.mode=="FT4"&&row.decode.utc==job.utcMs/1000;})==3);
  auto start=dx::monotonicMs();engine.ignoreTransmitter("K0ABC",true);CHECK(dx::monotonicMs()-start<100);for(const auto& rows:{engine.snapshot()->table,engine.snapshot()->map})for(const auto& row:rows)CHECK(row.decode.identity.transmitter!="K0ABC");CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.ignoredCalls.size()==1&&!s.moderating&&s.stats.messages==4&&s.tableCount==2&&s.mapCount==4;}));CHECK(engine.snapshot()->table.size()==2&&engine.snapshot()->map.size()==4&&engine.snapshot()->stats.activeSeconds>=uptime);for(const auto& rows:{engine.snapshot()->table,engine.snapshot()->map})for(const auto& row:rows)CHECK(row.decode.identity.transmitter!="K0ABC");CHECK(read(root/"receptions.csv").find("\"K0ABC\"")==std::string::npos&&read(root/"other.csv").find("\"K0ABC\"")==std::string::npos);
  // Persist a failed purge without deleting an unrelated file, retry on restart.
  auto blocked=root/"blocked.csv";write(blocked,"user file\n");{dx::Store store(root/"history.sqlite3");auto paths=store.getSetting("csv_paths",dx::Json::array());paths.push_back(blocked.string());store.setSetting("csv_paths",paths);}
  engine.ignoreTransmitter("K1ABC",true);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.stats.messages==2&&s.tableCount==1&&s.mapCount==2&&s.moderating&&!s.csvError.empty();}));CHECK(read(blocked)=="user file\n");for(const auto& rows:{engine.snapshot()->table,engine.snapshot()->map})for(const auto& row:rows)CHECK(row.decode.identity.transmitter=="K2ABC");engine.settings({{"history_all",false}});CHECK(waitFor(engine,[](const dx::Snapshot& s){return !s.historyAll;}));engine.active(false);
 }
 {dx::Store store(root/"history.sqlite3");auto cutoff=store.getSetting("log_purges",dx::Json::object()).at("K1ABC").get<int64_t>();auto ignored=reception(now,"CQ K1ABC FN42");ignored.identity=dx::meaning(ignored.message);write(root/"blocked.csv",dx::csvLogHeader()+dx::csvLogRow(cutoff,ignored));}
 {dx::Engine engine(root,mock,home,country);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.generation>0&&s.ignoredCalls.size()==2&&!s.moderating&&s.csvError.empty();}));CHECK(read(root/"blocked.csv")==dx::csvLogHeader());CHECK(engine.snapshot()->stats.messages==0&&engine.snapshot()->table.empty());engine.settings({{"stats_history",true}});CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.statsHistory&&s.stats.messages==2;}));engine.unignoreAll();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredCalls.empty();}));}
 auto unknownMock=folder/"dx_mock_valid_unknown";std::filesystem::create_symlink(std::filesystem::absolute(argv[1]),unknownMock);auto unknownRoot=folder/"unknown-async";auto blockedMessageLog=unknownRoot/"blocked.csv";
 {
  dx::Engine engine(unknownRoot,unknownMock,home,country);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.generation>0;}));dx::Job job;job.mode="FT8";job.dial=7074000;job.utcMs=int64_t(time(nullptr))*1000;job.pcm.resize(180000);job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);
  CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==3&&s.tableCount==3&&!s.csvCatchingUp;}));engine.settings({{"history_all",true}});CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.tableCount==3&&s.mapCount==1;}));
  engine.ignoreTransmitter("<1:123>",true);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredCalls.size()==1&&!s.moderating&&s.stats.messages==2&&s.tableCount==2&&s.map.empty();}));
  write(blockedMessageLog,"unrelated file\n");{dx::Store store(unknownRoot/"history.sqlite3");auto paths=store.getSetting("csv_paths",dx::Json::array());paths.push_back(blockedMessageLog.string());store.setSetting("csv_paths",paths);}
  engine.ignoreMessage("HELLO, \"WORLD\"",true);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredMessages.size()==1&&s.moderating&&!s.csvError.empty()&&s.stats.messages==1&&s.tableCount==1&&s.map.empty();}));CHECK(read(blockedMessageLog)=="unrelated file\n");CHECK(engine.snapshot()->table.front().decode.message=="OTHER UNKNOWN TEXT");
  job.utcMs+=15000;job.deadlineMs=dx::monotonicMs()+30000;engine.enqueue(job);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.stats.messages==2&&s.tableCount==1&&s.pendingWrites==0;}));CHECK(engine.snapshot()->table.front().count==2);
 }
 {dx::Store store(unknownRoot/"history.sqlite3");auto cutoff=store.getSetting("message_log_purges",dx::Json::object()).at("HELLO, \"WORLD\"").get<int64_t>();auto removed=reception(now,"HELLO, \"WORLD\"");write(blockedMessageLog,dx::csvLogHeader()+dx::csvLogRow(cutoff,removed));}
 {dx::Engine engine(unknownRoot,unknownMock,home,country);CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.historyAll&&s.stats.messages==2&&s.tableCount==1&&s.ignoredMessages.size()==1&&!s.moderating&&s.csvError.empty();}));CHECK(read(blockedMessageLog)==dx::csvLogHeader());CHECK(engine.snapshot()->table.front().decode.message=="OTHER UNKNOWN TEXT");engine.unignoreAll();CHECK(waitFor(engine,[](const dx::Snapshot& s){return s.ignoredCalls.empty()&&s.ignoredMessages.empty();}));}
 std::filesystem::remove_all(folder);std::cout<<"Latest message, unique stations, unresolved/message ignore, purge/restart, watermark and home-distance checks passed\n";return 0;
}
