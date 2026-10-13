// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
#include <atomic>
#include <filesystem>
#include <sys/types.h>
namespace dx {
class Helper {
public:
 explicit Helper(std::filesystem::path binary):binary_(std::move(binary)){}
 ~Helper(){stop();}
 Json decode(const Job& job,const std::atomic<bool>& stopping);
 void stop();pid_t pid()const{return pid_;}
private:
 void start();void transfer(int fd,void* data,size_t size,bool writing,int64_t deadline,const std::atomic<bool>& stopping);
 std::filesystem::path binary_;pid_t pid_=-1;int input_=-1,output_=-1;
};
int64_t monotonicMs();
}
