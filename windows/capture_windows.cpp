// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <cmath>
DXChannel::DXChannel(DXNotebook* p,dx::Preset preset_,double):owner(p),preset(std::move(preset_)),name("sharp_"+preset.mode+"_"+std::to_string(preset.dial)){}
void DXChannel::reset(){capture.reset();sequences.reset();phase_=0;started_=false;progress=0;}
void DXChannel::feed(const std::vector<std::complex<float>>& data,double rate,int64_t endMs,uint64_t sequence){
 if(!accept||data.empty()||!std::isfinite(rate)||std::abs(rate-12000)>.01){reset();return;}
 if(started_&&sequence!=sequence_+1){reset();++discontinuities;}
 sequence_=sequence;started_=true;
 // Restore USB audio after SDR# filtering/resampling. The 1500/3000 Hz
 // oscillators are exact eight/four-sample cycles at 12 kHz.
 constexpr float q=.7071067811865475f;
 constexpr std::complex<float> oscillator[]={{1,0},{q,q},{0,1},{-q,q},{-1,0},{-q,-q},{0,-1},{q,-q}};
 unsigned step=preset.mode=="FT8"||preset.mode=="FT4"?1:2;
 std::vector<float> audio;audio.reserve(data.size());double power=0;
 for(auto value:data){float sample=2*(value*oscillator[phase_]).real();phase_=(phase_+step)%8;audio.push_back(sample);power+=sample*sample;}
 level=10*std::log10(std::max(1e-12,power/audio.size()));streamMs=endMs;++blocks;
 if(preset.mode=="FT8"||preset.mode=="FT4"){
  auto job=capture.feed(audio.data(),audio.size(),endMs,preset.dial,preset.mode,preset.periodMs,preset.submode);samples=capture.samples();progress=capture.progress();
  if(job){if(replay)job->source="replay";owner->engine().enqueue(std::move(*job));}
 }else{
  auto jobs=sequences.feed(audio.data(),audio.size(),endMs,preset.dial,preset.mode,preset.periodMs,preset.submode);samples=sequences.samples();progress=sequences.progress();
  for(auto& job:jobs){if(replay)job.source="replay";owner->engine().enqueue(std::move(job));}
 }
}
