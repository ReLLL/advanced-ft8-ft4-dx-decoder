// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <ctime>
#include <filesystem>
namespace dx::platform {
inline tm* localTime(const time_t* value,tm* result){
#ifdef _WIN32
 return localtime_s(result,value)==0?result:nullptr;
#else
 return localtime_r(value,result);
#endif
}
inline tm* utcTime(const time_t* value,tm* result){
#ifdef _WIN32
 return gmtime_s(result,value)==0?result:nullptr;
#else
 return gmtime_r(value,result);
#endif
}
inline time_t utcEpoch(tm* value){
#ifdef _WIN32
 return _mkgmtime(value);
#else
 return timegm(value);
#endif
}
inline std::filesystem::path executable(std::filesystem::path value){
#ifdef _WIN32
 value+=".exe";
#endif
 return value;
}
}
