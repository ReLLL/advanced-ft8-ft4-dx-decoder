// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "corner_feed.h"
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"Failed: " #x <<" at "<<__LINE__<<"\n";return 1;}}while(0)
int main(){
 CHECK(dx::distanceDisplay(1609.344,true)==1000);CHECK(dx::distanceKilometers(1000,true)==1609.344);CHECK(dx::distanceDisplay(1234,false)==1234&&dx::distanceKilometers(1234,false)==1234);
 CHECK(dx::distanceSummary(1609.344,true)=="1000 mi"&&dx::distanceSummary(1609.344,false)=="1609 km");
 dx::CornerFeed feed;dx::Decode same;same.identity.transmitter="G1TEST";same.country="England";same.message="CQ G1TEST JN39";
 for(int i=1;i<=8;i++)feed.add(i,same,1000+i*100);
 CHECK(feed.rows().size()==8);CHECK(feed.rows().front().id==1&&feed.rows().back().id==8);
 feed.expire(11099,10);CHECK(feed.rows().size()==8);
 feed.expire(11100,10);CHECK(feed.rows().size()==7&&feed.rows().front().id==2);
 feed.expire(11800,10);CHECK(feed.rows().empty());
 feed.add(9,same,20000);dx::Decode other=same;other.identity.transmitter="K1TEST";feed.add(10,other,21000);
 feed.remove({"G1TEST",false});CHECK(feed.rows().size()==1&&feed.rows()[0].id==10);
 feed.remove({same.message,true});CHECK(feed.rows().empty());
 feed.add(11,same,30000);feed.expire(32000,0);CHECK(feed.rows().empty());
 feed.add(12,same,40000);feed.expire(159999,1000);CHECK(feed.rows().size()==1);feed.expire(160000,1000);CHECK(feed.rows().empty());
 feed.add(13,same,170000);feed.clear();CHECK(feed.rows().empty());
 std::cout<<"Per-reception feed, repeated messages, expiry, moderation and clear passed\n";
}
