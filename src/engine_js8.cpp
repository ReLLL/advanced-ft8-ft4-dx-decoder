// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
namespace dx {
void Engine::js8Loop(){
 while(!stopping_){Job job;{
  std::unique_lock<std::mutex> lock(mutex_);cv_.wait_for(lock,std::chrono::milliseconds(200),[&]{return stopping_||!js8Jobs_.empty()||js8Cancel_;});if(stopping_)break;
  if(js8Cancel_){js8Helper_.stop();js8Cancel_=false;}if(js8Jobs_.empty())continue;job=std::move(js8Jobs_.front());js8Jobs_.pop_front();if(monotonicMs()>job.deadlineMs){++status_.skipped;continue;}status_.js8Decoding=true;
 }
 try{auto response=js8Helper_.decode(job,js8Cancel_);std::vector<Decode> decoded;
  for(auto& row:response.at("results")){row["timestamp"]=job.utcMs/1000;row["slot_ms"]=job.utcMs;row["frequency"]=job.dial;row["mode"]="JS8";row["source"]=job.source;decoded.push_back(Decode::fromJson(row));}
  std::unique_lock<std::mutex> lock(mutex_);cv_.wait(lock,[&]{return stopping_||job.context!=modeContext_||writes_.size()+decoded.size()<=2000;});if(!stopping_&&job.context==modeContext_){for(auto& row:decoded)writes_.push_back(std::move(row));status_.skipped+=response.value("skipped",uint64_t(0));}status_.js8Error.clear();++status_.decodedJobs;
 }catch(const std::exception& e){std::lock_guard<std::mutex> lock(mutex_);if(!stopping_&&job.context==modeContext_){status_.js8Error="JS8: "+std::string(e.what());++status_.failures;++status_.skipped;}}
 {std::lock_guard<std::mutex> lock(mutex_);status_.js8Decoding=false;cv_.notify_all();}
 }js8Helper_.stop();js8Done_=true;cv_.notify_all();
}
}
