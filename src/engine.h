// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
#include "helper_client.h"
#include "alerts.h"
#include "csv_log.h"
#include "gridtracker.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <memory>
namespace dx {
struct Arrival {std::string key;int64_t observedMs=0;};
struct Snapshot {
 Stats stats;bool statsHistory=false;std::vector<Row> table,map;std::vector<Decode> details,hoverMessages;std::optional<Decode> hoverStation;
 std::vector<ExchangePath> paths;
 std::vector<std::string> mapHeardCountries;
 std::vector<Arrival> arrivals;int64_t newestReceptionUtc=0;ActivityBearing direction;std::vector<Decode> directionStations;
 std::string csvPath,csvError;uint64_t csvBytes=0;bool csvCatchingUp=false;
 GridTrackerState gridTracker;
 int64_t tableCount=0,mapCount=0,tableExcluded=0,mapExcluded=0;
 Json settings=Json::object();uint64_t generation=0;double queryMs=0;
 std::string error,notice,alert,alertTone;int64_t alertUtc=0;bool alertSound=false;size_t pendingWrites=0,queued=0;uint64_t skipped=0,failures=0,decodedJobs=0;
 double lastDecodeMs=0;int helperPid=-1;bool decoding=false;
};
class Engine {
public:
 Engine(std::filesystem::path root,std::filesystem::path helper,Point home,
  std::function<std::pair<std::string,std::string>(const std::string&)> country);
 ~Engine();void enqueue(Job job);void query(Query table,Query map,uint64_t generation);
 void select(const Decode& d,int offset=0);void settings(Json values);void exportData(Query query,std::filesystem::path path,bool csv);
 void active(bool enabled);void retry();std::shared_ptr<const Snapshot> snapshot()const;
 void alertContext(std::string bands,bool ft8,bool ft4);
 void home(Point point);
 void resetStats();
 void gridTrackerContext(GridTrackerContext context);
 void hover(const Decode& station);
private:
 void decodeLoop();void storeLoop();void importText(Store& store);void publish(Store& store);
 std::filesystem::path root_;Helper helper_;Point home_;
 std::function<std::pair<std::string,std::string>(const std::string&)> country_;
 mutable std::mutex mutex_;std::condition_variable cv_;std::thread decoder_,writer_;std::atomic<bool> stopping_{false};
 std::deque<Job> jobs_;std::deque<Decode> writes_;std::shared_ptr<const Snapshot> snapshot_=std::make_shared<Snapshot>();
 Query table_,map_;uint64_t generation_=1;bool queryDirty_=true,active_=false,retry_=false,blocked_=false;
 std::optional<Decode> selection_;int historyOffset_=0;bool selectionDirty_=false;
 std::optional<Decode> hovered_;
 Json pendingSettings_=Json::object();std::optional<std::tuple<Query,std::filesystem::path,bool>> export_;
 Snapshot status_;std::string allTxt_;int64_t logPosition_=0;uint64_t logInode_=0;std::string logAnchor_;
 std::optional<Point> pendingHome_;
 bool pendingStatsReset_=false;int64_t statsAfterId_=0,statsSince_=0;double sessionSeconds_=0;
 CsvLog csv_;int64_t arrivalCursor_=0,startedAt_=0,startAfterId_=0;std::atomic<bool> decoderDone_{false};
 GridTracker gridTracker_;GridTrackerContext gridTrackerContext_;
 std::string alertBands_;bool alertFT8_=true,alertFT4_=true;Alerts alerts_;int64_t lastBaseline_=0;
};
// Complete UTC slots only. A timestamp discontinuity or reset invalidates partial data.
class SlotCapture {
public:
 std::optional<Job> feed(const float* samples,size_t count,int64_t endMs,int64_t dial,const std::string& mode);
 void reset(){pcm_.clear();slot_=-1;nextSample_=-1;}
 double progress()const{return pcm_.size()/double(periodSamples_);}
 uint64_t discontinuities()const{return discontinuities_;}uint64_t samples()const{return samples_;}
private:
 int64_t slot_=-1,nextSample_=-1;size_t periodSamples_=180000;std::vector<int16_t> pcm_;
 uint64_t discontinuities_=0,samples_=0;
};
}
