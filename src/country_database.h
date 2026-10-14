// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace dx {
struct CountryDatabase {
 struct Prefix {bool exact=false;std::string value;};
 struct DXCC {std::string name,continent;std::vector<Prefix> prefixes;};
 std::vector<DXCC> dxcc;
 void load(const std::filesystem::path& path);
};
}
