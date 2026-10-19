// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "engine.h"
#include <imgui.h>
#include <complex>
namespace dx {struct SharpAudio;}
class DXNotebook;
struct DXChannel {
 DXNotebook* owner;dx::Preset preset;std::string name;
 dx::SlotCapture capture;dx::SequenceCapture sequences;std::atomic<bool> accept{false},replay{false};
 std::atomic<uint64_t> blocks{0},samples{0},discontinuities{0};std::atomic<double> progress{0},level{-120};std::atomic<int64_t> streamMs{0};
 DXChannel(DXNotebook*,dx::Preset,double);
 void feed(const std::vector<std::complex<float>>&,double,int64_t,uint64_t);
 void reset();
private:
 unsigned phase_=0;uint64_t sequence_=0;bool started_=false;
};
