// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "model.h"
namespace dx {
enum class CleanupKind {Before,After,Between,Country,ExceptCountry,MissingDistance,MissingGrid};
struct SavedCleanup {
 CleanupKind kind=CleanupKind::Before;int64_t from=0,to=0;std::string country;bool includeCached=false;
 bool valid()const;bool matches(const Decode& decode)const;
 Json json()const;static SavedCleanup fromJson(const Json& json);
 std::string description()const;
};
std::optional<int64_t> cleanupDate(const std::string& text,bool endOfDay=false);
}
