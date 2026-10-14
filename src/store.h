// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include <sqlite3.h>
#include <filesystem>
#include <functional>
namespace dx {
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
 int64_t ignoreTransmitter(const std::string& call);
 int64_t ignoreMessage(const std::string& message);bool ignoredMessage(const std::string& message);std::vector<std::string> ignoredMessages();bool ignoredReception(const Decode& decode);
 void unignoreAll();std::vector<std::string> ignoredCalls();bool ignored(const std::string& call);
private:
 int64_t ignore(const IgnoreTarget& target);
 void exec(const std::string& sql);sqlite3* db_=nullptr;
};
}
