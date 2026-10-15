// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto path=(std::filesystem::temp_directory_path()/"dx-reset-defaults-XXXXXX").string();CHECK(mkdtemp(path.data()));TestFolderCleanup cleanup(path);auto file=std::filesystem::path(path)/"history.sqlite3";dx::Decode d;d.utc=time(nullptr)-60;d.mode="FT8";d.dial=14074000;d.message="CQ DL8OH JO33";auto home=*dx::gridPoint("KM69");
 {dx::Store store(file);CHECK(store.insert(d,home));store.ignoreTransmitter("K1ABC");store.setSetting("ui",{{"history_all",true},{"ft8",false},{"map",{{"snr_colors",false}}},{"history_max_records",20000}});}
 {dx::Engine engine(path,"/tmp/unused-reset-helper",home,[](const auto&){return std::make_pair(std::string{},std::string{});});auto wait=[&](auto predicate){auto until=dx::monotonicMs()+5000;while(dx::monotonicMs()<until){if(predicate(*engine.snapshot()))return true;std::this_thread::sleep_for(std::chrono::milliseconds(10));}return predicate(*engine.snapshot());};CHECK(wait([](const auto& s){return s.generation>0&&s.historyAll;}));auto revision=engine.restoreDefaults();CHECK(wait([&](const auto& s){return s.settings.value("defaults_revision",uint64_t(0))==revision&&!s.historyAll;}));auto s=engine.snapshot();CHECK(s->settings.size()==1&&s->savedMessages==1&&s->retention.limit==100000&&s->retention.target==95000&&s->ignoredCalls.size()==1);}
 {dx::Store store(file);CHECK(store.savedMessageCount()==1&&store.ignored("K1ABC"));CHECK(store.logRows(0)[0].second.json()==d.json());}
 std::cout<<"Atomic preference reset preserves reception payloads and moderation rules\n";
}
