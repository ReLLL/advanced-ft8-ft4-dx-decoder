// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "modes.h"
#include <iostream>
#include <map>
#include <set>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 CHECK(dx::sequencePeriods("FST4").size()==7&&dx::sequencePeriods("FST4W").size()==4&&dx::sequencePeriods("Q65").size()==5);
 CHECK(dx::sequencePeriods("FST4",60000)==std::vector<int>{60000});CHECK(dx::sequencePeriods("FST4W",60000).empty());
 CHECK(dx::sequenceSubmodeMaximum(0)==2&&dx::sequenceSubmodeMaximum(30000)==3&&dx::sequenceSubmodeMaximum(60000)==4);
 std::vector<float> second(12000,.125f);
 for(const char* mode:{"FST4","FST4W","Q65","WSPR","JS8"}){
  dx::SequenceCapture capture;std::map<int,int> counts;std::deque<dx::Job> together;auto periods=dx::sequencePeriods(mode);
  for(int64_t t=1;t<=1801;t++){
   auto jobs=capture.feed(second.data(),second.size(),t*1000,14095600,mode);
   if(t==1800)for(const auto& job:jobs)together.push_back(job);
   for(const auto& job:jobs){++counts[job.periodMs];CHECK(job.pcm.empty()&&!job.audio.empty());CHECK(job.utcMs%job.periodMs==0);CHECK(job.utcMs+job.periodMs==t*1000);
    int64_t cursor=job.utcMs*12,end=(job.utcMs+job.periodMs)*12;size_t samples=0;
    for(const auto& block:job.audio){auto first=std::max(cursor,block->startSample),last=std::min(end,block->startSample+int64_t(block->pcm.size()));CHECK(first==cursor&&last>first);CHECK(block->pcm.front()==2047&&block->pcm.back()==2047);samples+=size_t(last-first);cursor=last;}
    CHECK(cursor==end&&samples==size_t(job.periodMs)*12);
   }
   CHECK(capture.bufferedBytes()<=size_t(periods.back())*24+24000);
  }
  for(int period:periods)CHECK(counts[period]==1800000/period);
  CHECK(together.size()==periods.size());CHECK(dx::queuedAudioBytes(together)==size_t(periods.back())*24);
  auto saved=together.back().audio.front();capture.reset();CHECK(capture.bufferedBytes()==0&&saved->pcm.front()==2047);
 }
 // Starting inside a sequence must not decode its partial prefix. UTC 300 s
 // must still trigger even though it is not a 120-second boundary.
 dx::SequenceCapture capture;std::map<int,int> counts;
 for(int64_t t=2;t<=600;t++){auto jobs=capture.feed(second.data(),second.size(),t*1000,14095600,"FST4W");for(const auto& job:jobs){CHECK(job.utcMs>=1000);++counts[job.periodMs];}}
 CHECK(counts[120000]==4&&counts[300000]==1&&counts[900000]==0);
 CHECK(capture.feed(second.data(),second.size(),900000,14095600,"FST4W").empty());CHECK(capture.discontinuities()==1);
 CHECK(capture.feed(second.data(),second.size(),901000,14095600,"FST4W",300000).empty());
 // Real receiver chunks straddle millisecond/UTC boundaries. Compare their
 // exact windows with the one-second feed, including NaN sanitization.
 dx::SequenceCapture chunks;std::vector<float> input(516,.125f);std::set<int> seen;
 for(int64_t at=0;at<3600000;){size_t n=size_t(std::min<int64_t>(516,3600000-at));auto jobs=chunks.feed(input.data(),n,(at+int64_t(n))/12,474200,"FST4");at+=int64_t(n);for(const auto& job:jobs)seen.insert(job.periodMs);}
 CHECK(seen.count(15000)&&seen.count(30000)&&seen.count(60000)&&seen.count(120000)&&seen.count(300000));CHECK(chunks.discontinuities()==0);
 std::cout<<"All UTC periods, partial starts, overlapping audio sharing, bounded memory and discontinuity resets passed\n";return 0;
}
