// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <tuple>
namespace dx {
void openMapCredit(int index);
struct TileKey {int z,x,y,layer=1;bool operator<(const TileKey& b)const{return std::tie(layer,z,x,y)<std::tie(b.layer,b.z,b.x,b.y);}bool operator==(const TileKey& b)const{return layer==b.layer&&z==b.z&&x==b.x&&y==b.y;}};
struct TileStats {uint64_t requests=0,diskHits=0,diskBytes=0;size_t resident=0,wanted=0,ready=0;double viewportMs=0;};
class Tiles {
public:
 explicit Tiles(std::filesystem::path cache);~Tiles();
 void visible(const std::vector<TileKey>& keys);
 unsigned int texture(TileKey key);
 std::vector<TileKey> ready();
 std::string status();
 TileStats stats();
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
