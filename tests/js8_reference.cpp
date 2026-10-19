// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "model.h"
#include <JS8.hpp>
#include <commons.h>
#include <varicode.h>
#include <decodedtext.h>
#include <iostream>
#include <cmath>
#include <random>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
int main(int argc,char** argv){
#ifdef _WIN32
 _setmode(1,_O_BINARY);
#endif
 if(argc!=5)return 2;int period=std::stoi(argv[1]),submode=period==15?0:period==10?1:period==6?2:period==30?4:-1;if(submode<0)return 2;
 const int nsps=period==15?1920:period==10?1200:period==6?600:3840,start=period==15||period==30?6000:period==10?2400:1200;
 auto frames=Varicode::buildMessageFrames(QString::fromUtf8(argv[2]),QString::fromUtf8(argv[3]),{},QString::fromUtf8(argv[4]),true,false,submode);if(frames.empty())return 3;
 dx::Json metadata=dx::Json::array();std::vector<int16_t> pcm;std::mt19937 random(42);std::normal_distribution<float> noise(0,100);
 for(const auto& frame:frames){int tones[79];JS8::encode(frame.second,JS8::Costas::array(period==15?JS8::Costas::Type::ORIGINAL:JS8::Costas::Type::MODIFIED),frame.first.toStdString().c_str(),tones);metadata.push_back({{"data",frame.first.toStdString()},{"bits",frame.second}});double phase=0;
  for(int i=0;i<period*12000;i++){int symbol=(i-start)/nsps;double signal=0;if(i>=start&&symbol<79){phase+=2*3.141592653589793*(1500.+tones[symbol]*12000./nsps)/12000.;signal=12000*std::sin(phase);}pcm.push_back(int16_t(std::clamp(signal+noise(random),-32767.,32767.)));}
 }
 auto header=dx::Json{{"period",period},{"frames",metadata}}.dump();uint32_t size=header.size();std::cout.write(reinterpret_cast<const char*>(&size),4);std::cout.write(header.data(),header.size());std::cout.write(reinterpret_cast<const char*>(pcm.data()),pcm.size()*2);return std::cout?0:4;
}
