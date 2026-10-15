// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <fstream>
#include <thread>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(int argc,char** argv){
 if(argc!=3)return 2;std::filesystem::path helper=std::filesystem::absolute(argv[1]),fixtures=std::filesystem::absolute(argv[2]);
 auto root=fixtures/"saved";dx::Engine engine(root,helper,*dx::gridPoint("KM69KW"),[](const std::string& call){return std::make_pair(call=="K1ABC"?"United States":"",call=="K1ABC"?"NA":"");});
 int sequence=0;for(const char* mode:{"FST4","FST4W","Q65","WSPR"}){
  dx::Job job;job.mode=mode;job.source="test";job.utcMs=int64_t(time(nullptr))*1000;job.dial=14095600;job.periodMs=job.mode=="FST4W"||job.mode=="WSPR"?120000:15000;job.deadlineMs=dx::monotonicMs()+120000;
  std::ifstream audio(fixtures/(job.mode+".pcm"),std::ios::binary);CHECK(audio);job.pcm.resize(size_t(job.periodMs)*12);audio.read(reinterpret_cast<char*>(job.pcm.data()),std::streamsize(job.pcm.size()*2));CHECK(audio);
  // Exercise the production shared-block IPC path with exact float/PCM round trip.
  dx::SequenceCapture capture;std::vector<float> samples(job.pcm.size());for(size_t i=0;i<samples.size();i++)samples[i]=float(job.pcm[i])/16383.52f;
  // Simulated, increasing UTC slots after session start; no wall-clock waiting.
  int64_t aligned=(job.utcMs/600000+1)*600000+sequence++*120000;std::vector<dx::Job> captured;
  for(size_t at=0;at<samples.size();at+=12000){auto ready=capture.feed(samples.data()+at,std::min<size_t>(12000,samples.size()-at),aligned+int64_t(at+12000)/12,job.dial,job.mode,job.periodMs,job.submode);for(auto& item:ready)captured.push_back(std::move(item));}
  CHECK(captured.size()==1&&captured.front().pcm.empty());job=std::move(captured.front());job.source="test";
  auto before=engine.snapshot()->stats.messages;engine.enqueue(std::move(job));auto start=dx::monotonicMs();while(engine.snapshot()->stats.messages==before&&dx::monotonicMs()-start<60000)std::this_thread::sleep_for(std::chrono::milliseconds(20));
  auto snapshot=engine.snapshot();CHECK(snapshot->modeError.empty());CHECK(snapshot->stats.messages==before+1);CHECK(snapshot->table.size()==1);auto d=snapshot->table.front().decode;CHECK(d.mode==mode&&d.identity.transmitter=="K1ABC"&&d.country=="United States"&&d.km&&*d.km>1000);CHECK(d.identity.grid=="FN42");
  if(d.mode=="WSPR"||d.mode=="FST4W"){CHECK(d.identity.called.empty()&&d.identity.type=="Beacon"&&d.txPowerDbm);CHECK(snapshot->paths.empty());}
  std::cout<<d.mode<<" worker, identity, country, home distance, map/table and persistence passed\n";
 }
 dx::Query q;q.messages=true;q.filter.messageKind=dx::MessageKind::Beacon;engine.query(q,q,42);auto start=dx::monotonicMs();while(engine.snapshot()->generation!=42&&dx::monotonicMs()-start<5000)std::this_thread::sleep_for(std::chrono::milliseconds(20));CHECK(engine.snapshot()->tableCount==2&&engine.snapshot()->mapCount==2);
 engine.invalidateAdditional();return 0;
}
