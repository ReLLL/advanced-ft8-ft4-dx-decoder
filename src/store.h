// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include "saved_cleanup.h"
#include <sqlite3.h>
#include <filesystem>
#include <functional>
#include <map>
namespace dx {
constexpr int defaultHistoryLimit=100000;
constexpr int64_t historyCheckIntervalMs=30*60*1000;
constexpr int64_t historyCleanupTarget(int64_t limit){return limit*95/100;}
class Store {
public:
 explicit Store(const std::filesystem::path& file);~Store();
 Store(const Store&)=delete;Store& operator=(const Store&)=delete;
 bool insert(Decode& d,Point home,const std::function<std::pair<std::string,std::string>(const std::string&)>& country={});
 std::vector<Row> rows(const Query& query,int offset=0,int limit=10000,int64_t* countOnly=nullptr);
 std::vector<Decode> history(const std::string& call,int64_t dial,const std::string& mode,int offset=0);
 std::vector<Decode> uniqueMessages(const Decode& station,const Query& view);
 Stats stats(int64_t now);int64_t importBridge(const std::filesystem::path&,Point,const std::function<std::pair<std::string,std::string>(const std::string&)>&);
 Stats sessionStats(int64_t now,int64_t afterRowId,int64_t since);
 Json getSetting(const std::string& key,const Json& fallback);void setSetting(const std::string& key,const Json& value);
 void exportRows(const Query& query,const std::filesystem::path& path,bool csv);
 void reclassifyCountries(const std::function<std::pair<std::string,std::string>(const std::string&)>& country);
 void rebaseDistances(Point home);
 std::vector<ExchangePath> exchangePaths(const Query& query);
 std::vector<std::pair<int64_t,Decode>> logRows(int64_t after,int limit=256);
 int64_t lastReceptionId();
 int64_t ignoreTransmitter(const std::string& call,bool deleteRecords=false);
 int64_t ignoreMessage(const std::string& message,bool deleteRecords=false);bool ignoredMessage(const std::string& message);std::vector<std::string> ignoredMessages();bool ignoredReception(const Decode& decode);
 int64_t deleteTransmitter(const std::string& call,bool ignoreFuture=false);
 int64_t deleteMessage(const std::string& message,bool ignoreFuture=false);
 int64_t moderate(const IgnoreTarget& target,bool deleteRecords,bool ignoreFuture);
 int64_t countCleanup(const SavedCleanup& cleanup,int64_t throughId);
 int64_t deleteSaved(const SavedCleanup& cleanup,int64_t throughId);
 std::vector<std::string> savedCountries();
 int64_t savedMessageCount();
 void compact();
 struct RetentionState {int64_t firstUtc=0,lastUtc=0,deleted=0,limit=defaultHistoryLimit;bool enabled=true,running=false;};
 RetentionState retentionState();
 // One bounded transaction. The caller must drain reception/CSV work first.
 int64_t retain(int64_t limit,int batch=128,int64_t syncedThrough=INT64_MAX);
 bool reclaimPages(int pages=32);
 void unignoreAll();std::vector<std::string> ignoredCalls();bool ignored(const std::string& call);
private:
 void ensureReceptionIds();void ensureEntities();void rebuildSummaries(bool stations=true);
 void invalidateSession(){sessionAfter_=-1;}
 void expireSession(const std::vector<std::pair<int64_t,Decode>>& expired);
 void exec(const std::string& sql);sqlite3* db_=nullptr;
 int64_t sessionAfter_=-1,sessionSince_=0,sessionCursor_=0,sessionDataVersion_=-1,sessionFarthestId_=0;
 Stats session_;std::map<std::string,std::pair<int64_t,Decode>> sessionCalls_,sessionCountries_;
 std::map<std::string,int64_t> sessionCountryFirst_;
};
}
