// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
extern "C" void dx_native_decode(int,int,int,int,const int16_t*,int,int,int);
extern "C" void dx_native_fixture(int,int,int,int16_t*,int);
static bool found=false;
extern "C" void dx_mode_result(int snr,double dt,double freq,int used,const char* text,int length){
 std::string message(text,size_t(length));std::cerr<<message<<" | "<<snr<<" dB | "<<freq<<" Hz | "<<dt<<" s | "<<used<<" intervals\n";
 if(message.find("K1ABC")!=std::string::npos)found=true;
}
int main(int argc,char**argv){
 if(argc!=4&&argc!=5)return 2;int mode=std::atoi(argv[1]),period=std::atoi(argv[2]),submode=std::atoi(argv[3]);
 if(mode<1||mode>3||period<15||period>1800||submode<0||submode>4)return 2;
 std::vector<int16_t> samples(size_t(period)*12000);
 dx_native_fixture(mode,period,submode,samples.data(),int(samples.size()));
 if(argc==5){std::ofstream output(argv[4],std::ios::binary);output.write(reinterpret_cast<const char*>(samples.data()),std::streamsize(samples.size()*2));return output?0:2;}
 dx_native_decode(mode,period,submode,1200,samples.data(),int(samples.size()),0,1);
 return found?0:1;
}
