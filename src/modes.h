// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <optional>
#include <string_view>
#include <cstdint>
#include <vector>
namespace dx {
enum class MarkerShape {Circle,Square,Triangle,Diamond,Hexagon,Octagon};
struct ModeSpec {
 std::string_view name;MarkerShape marker;bool beacon=false,available=false;
 std::array<int,7> periodsMs{};size_t periodCount=0;
};
const std::array<ModeSpec,7>& modeSpecs();
const ModeSpec* modeSpec(std::string_view name);
std::optional<int64_t> capturePeriodMs(std::string_view mode,int requestedMs=0);
// Zero selects every supported period, independently of the legacy single-slot API.
std::vector<int> sequencePeriods(std::string_view mode,int selectionMs=0);
int sequenceSubmodeMaximum(int selectionMs);
MarkerShape modeMarker(std::string_view mode);
// Circle uses the existing native circle renderer; polygons use these local offsets.
struct MarkerOutline {std::array<std::array<float,2>,8> points{};int count=0;};
MarkerOutline markerOutline(MarkerShape shape,float radius);
}
