// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <optional>
#include <string_view>
#include <cstdint>
namespace dx {
enum class MarkerShape {Circle,Square,Triangle,Diamond,Hexagon};
struct ModeSpec {
 std::string_view name;MarkerShape marker;bool beacon=false,available=false;
 std::array<int,7> periodsMs{};size_t periodCount=0;
};
const std::array<ModeSpec,6>& modeSpecs();
const ModeSpec* modeSpec(std::string_view name);
std::optional<int64_t> capturePeriodMs(std::string_view mode,int requestedMs=0);
MarkerShape modeMarker(std::string_view mode);
// Circle uses the existing native circle renderer; polygons use these local offsets.
struct MarkerOutline {std::array<std::array<float,2>,6> points{};int count=0;};
MarkerOutline markerOutline(MarkerShape shape,float radius);
}
