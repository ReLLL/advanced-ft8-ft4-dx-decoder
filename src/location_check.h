// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "cartography.h"
#include <filesystem>
#include <map>
namespace dx {
class LocationCheck {
public:
 bool load(const std::filesystem::path& file);
 bool mismatch(const std::string& country,const std::string& grid,const std::string& call,double toleranceKm=300);
 bool ready()const{return !areas_.empty();}
private:
 struct Area {std::vector<std::vector<std::vector<Point>>> polygons;std::vector<std::pair<Point,Point>> bounds;};
 std::vector<Area> areas_;std::map<std::string,size_t> keys_;std::map<std::string,bool> cache_;
};
}
