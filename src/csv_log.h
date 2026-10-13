// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.h"
namespace dx {
std::string csvLogHeader();
std::string csvLogRow(int64_t id,const Decode& decode);
class CsvLog {
public:
 bool sync(Store& store,const std::filesystem::path& path,uint64_t maximumBytes);
 uint64_t bytes()const{return bytes_;}
 int64_t cursor()const{return cursor_;}
private:
 void recover();void trim(uint64_t maximumBytes);
 std::filesystem::path path_;uint64_t bytes_=0;int64_t cursor_=0;bool ready_=false;
};
}
