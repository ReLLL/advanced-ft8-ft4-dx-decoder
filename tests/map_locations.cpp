// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "store.h"
#include "temporary_folder.h"
#include <iostream>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(0)
int main(){
 auto path=(std::filesystem::temp_directory_path()/"dx-map-locations-XXXXXX").string();CHECK(mkdtemp(path.data()));TestFolderCleanup cleanup(path);dx::Store store(std::filesystem::path(path)/"history.sqlite3");auto home=*dx::gridPoint("KM69");int64_t utc=1800000000;
 auto put=[&](int64_t timestamp,const std::string& message){dx::Decode d;d.utc=timestamp;d.slotMs=timestamp*1000;d.dial=14074000;d.mode="FT8";d.message=message;d.source="live";if(!store.insert(d,home))throw std::runtime_error("fixture insert failed");return d;};
 auto grid=put(utc,"CQ DL8OH JO33");auto week=put(utc+dx::locationCacheSeconds,"JH1DSX DL8OH -03");CHECK(week.gridSource=="cached"&&week.gridUtc==utc&&week.position&&week.km&&!week.distanceRecord);
 auto expired=put(utc+dx::locationCacheSeconds+15,"BG2EFX DL8OH RR73");CHECK(!expired.position&&!expired.km&&expired.identity.grid.empty());
 dx::Query q;q.uniqueCalls=true;q.since=expired.utc;auto rows=store.rows(q);CHECK(rows.size()==1&&!rows.front().mapPoint());const auto payload=rows.front().decode.json();store.mapLocations(rows);CHECK(rows.front().lastKnownLocation&&rows.front().mapPoint());CHECK(rows.front().lastKnownLocation->grid=="JO33"&&rows.front().lastKnownLocation->utc==utc);CHECK(rows.front().decode.json()==payload&&!rows.front().decode.km);
 // A later grid must not leak into earlier historical views.
 put(expired.utc+60,"CQ DL8OH JO44");store.mapLocations(rows);CHECK(rows.front().lastKnownLocation->grid=="JO33");
 // Country-only stations and reused unresolved hashes cannot invent locations.
 auto unknown=put(expired.utc+90,"CQ DL1ABC");dx::Row unlocated{unknown};dx::Row hash{expired};hash.decode.identity.transmitter="<1:123>";std::vector<dx::Row> missing{unlocated,hash};store.mapLocations(missing);CHECK(!missing[0].mapPoint()&&!missing[1].mapPoint());
 // Deleting a transmitter also removes its location evidence.
 store.deleteTransmitter("DL8OH");store.mapLocations(rows);CHECK(!rows.front().lastKnownLocation&&!rows.front().mapPoint());
 std::cout<<"Seven-day grid evidence, map-only last-known locations, no future/hash guesses, deletion and unchanged reception payloads\n";
}
