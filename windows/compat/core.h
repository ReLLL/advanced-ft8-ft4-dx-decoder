// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sharp_host.h"
// Only UI preferences are adapted here; no SDR++ host ABI is loaded on Windows.
namespace core {
struct Settings {
 dx::Json conf={{"moduleInstances",dx::Json::object()}};std::mutex mutex;
 void acquire(){mutex.lock();}void release(bool save=false);
 void load();
};
inline Settings configManager;
}
namespace gui {
struct MainWindow {bool isPlaying()const{return dx::sharpHost().playing;}};
struct Waterfall {
 double getCenterFrequency()const{return dx::sharpHost().center;}
 // SDR# exposes the RF capture span, not the waterfall's private pan offset.
 // The listening VFO may even be outside a recording's RF span.
 double getViewOffset()const{return 0;}
 double getViewBandwidth()const{return dx::sharpHost().width;}
};
inline MainWindow mainWindow;inline Waterfall waterfall;
}
