// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include <set>
namespace dx {
uint64_t Engine::previewCleanup(SavedCleanup criteria){if(!criteria.valid())return 0;std::lock_guard<std::mutex> lock(mutex_);auto id=++nextCleanup_;cleanupPreview_=CleanupRequest{std::move(criteria),0,id};cv_.notify_all();return id;}
uint64_t Engine::deleteSaved(SavedCleanup criteria,int64_t throughId){if(!criteria.valid()||throughId<0)return 0;std::lock_guard<std::mutex> lock(mutex_);auto id=++nextCleanup_;cleanups_.push_back({std::move(criteria),throughId,id});status_.cleanupBusy=true;status_.cleanupError.clear();cleanupRetryAt_=0;cv_.notify_all();return id;}
bool Engine::manageSaved(Store& store){
 std::optional<CleanupRequest> preview,request;{std::lock_guard<std::mutex> lock(mutex_);preview=std::move(cleanupPreview_);cleanupPreview_.reset();if(!cleanups_.empty())request=cleanups_.front();}bool dirty=false;
 if(preview){auto through=store.lastReceptionId(),count=store.countCleanup(preview->criteria,through);std::lock_guard<std::mutex> lock(mutex_);status_.cleanupPreviewId=preview->id;status_.cleanupPreview=preview->criteria.json();status_.cleanupMatches=count;status_.cleanupThroughId=through;dirty=true;}
 if(request){auto deleted=store.deleteSaved(request->criteria,request->throughId);std::lock_guard<std::mutex> lock(mutex_);cleanups_.pop_front();status_.completedCleanup=request->id;status_.cleanupDeleted=deleted;++status_.noticeRevision;status_.notice="Deleted "+std::to_string(deleted)+" saved receptions; cleaning logs and compacting history";status_.arrivals.clear();status_.alert.clear();cornerFeed_.clear();if(selection_&&request->criteria.matches(*selection_))selection_.reset();if(hovered_&&request->criteria.matches(*hovered_))hovered_.reset();queryDirty_=true;dirty=true;}
 auto jobs=store.getSetting("cleanup_jobs",Json::object());if(jobs.empty()&&!store.getSetting("compact_pending",false).get<bool>()){std::lock_guard<std::mutex> lock(mutex_);status_.cleanupBusy=!cleanups_.empty();return dirty;}
 {std::lock_guard<std::mutex> lock(mutex_);status_.cleanupBusy=true;}
 if(monotonicMs()<cleanupRetryAt_)return dirty;
 try{
  bool compact=store.getSetting("compact_pending",false).get<bool>();for(const auto& entry:jobs.items())compact=compact||entry.value().value("compact_pending",false);
  if(compact){store.compact();store.setSetting("compact_pending",false);for(auto& entry:jobs.items())entry.value()["compact_pending"]=false;store.setSetting("cleanup_jobs",jobs);}
  for(const auto& entry:jobs.items()){
   auto criteria=SavedCleanup::fromJson(entry.value().at("criteria"));auto through=entry.value().at("through_id").get<int64_t>();std::set<std::string> paths;
   for(const auto& path:entry.value().value("paths",Json::array()))if(path.is_string())paths.insert(path.get<std::string>());
   paths.insert((root_/"receptions.csv").string());
   for(const auto& path:paths)csv_.purgeSaved(store,path,criteria,through);
   auto remaining=store.getSetting("cleanup_jobs",Json::object());remaining.erase(entry.key());store.setSetting("cleanup_jobs",remaining);dirty=true;
  }
  std::lock_guard<std::mutex> lock(mutex_);status_.cleanupBusy=!cleanups_.empty();status_.cleanupError.clear();++status_.noticeRevision;status_.notice="Saved reception cleanup complete; history compacted";
 }catch(const std::exception& e){std::lock_guard<std::mutex> lock(mutex_);status_.cleanupError=e.what();cleanupRetryAt_=monotonicMs()+5000;dirty=true;}
 return dirty;
}
}
