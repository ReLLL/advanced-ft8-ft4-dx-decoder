// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "modes.h"
#include <cmath>
#include <unordered_set>
namespace dx {
size_t SequenceCapture::bufferedBytes()const{size_t bytes=0;for(const auto& block:audio_)bytes+=block->pcm.size()*sizeof(int16_t);return bytes;}
std::vector<Job> SequenceCapture::feed(const float* samples,size_t count,int64_t endMs,int64_t dial,const std::string& mode,int selectionMs,int submode){
 std::vector<Job> jobs;auto periods=sequencePeriods(mode,selectionMs);
 if(periods.empty()||endMs<0||endMs>INT64_MAX/12||count>1000000){reset();return jobs;}
 if(mode_!=mode||dial_!=dial||selectionMs_!=selectionMs||submode_!=submode)reset();
 mode_=mode;dial_=dial;selectionMs_=selectionMs;submode_=submode;samples_+=count;
 int64_t start=endMs*12-int64_t(count);if(start<0){reset();return jobs;}
 if(nextSample_>=0){if(std::abs(start-nextSample_)>2400){++discontinuities_;reset();}else start=nextSample_;}
 if(firstSample_<0)firstSample_=start;
 const int64_t shortest=int64_t(periods.front())*12,longest=int64_t(periods.back())*12;
 size_t offset=0;
 while(offset<count){
  int64_t at=start+int64_t(offset),boundary=INT64_MAX;for(int period:periods){auto window=int64_t(period)*12;boundary=std::min(boundary,(at/window+1)*window);}
  size_t length=std::min({count-offset,size_t(boundary-at),size_t(12000)});
  auto block=std::make_shared<AudioBlock>();block->startSample=at;block->pcm.resize(length);
  for(size_t i=0;i<length;i++){float v=std::isfinite(samples[offset+i])?samples[offset+i]:0;block->pcm[i]=int16_t(std::clamp(v*16383.52f,-32767.f,32767.f));}
  audio_.push_back(std::move(block));offset+=length;int64_t end=at+int64_t(length);
  // Check each UTC boundary: 120-second and 300-second windows are not nested.
  for(int period:periods){int64_t window=int64_t(period)*12;
   if(end%window||firstSample_>end-window)continue;
   Job job;job.utcMs=(end-window)/12;job.dial=dial;job.mode=mode;job.periodMs=period;job.submode=submode;job.deadlineMs=monotonicMs()+period;
   for(const auto& part:audio_)if(part->startSample+int64_t(part->pcm.size())>end-window&&part->startSample<end)job.audio.push_back(part);
   jobs.push_back(std::move(job));
  }
  while(!audio_.empty()&&audio_.front()->startSample+int64_t(audio_.front()->pcm.size())<=end-longest)audio_.pop_front();
 }
 nextSample_=start+int64_t(count);auto slot=nextSample_/shortest*shortest;
 progress_=firstSample_<=slot?double(nextSample_%shortest)/double(shortest):0;
 return jobs;
}
size_t queuedAudioBytes(const std::deque<Job>& jobs,const Job* extra){
 size_t bytes=0;std::unordered_set<const AudioBlock*> seen;
 auto add=[&](const Job& job){bytes+=job.pcm.size()*sizeof(int16_t);for(const auto& block:job.audio)if(seen.insert(block.get()).second)bytes+=block->pcm.size()*sizeof(int16_t);};
 for(const auto& job:jobs)add(job);if(extra)add(*extra);return bytes;
}
}
