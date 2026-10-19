// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "gridtracker.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(0)
int main(){try{
 WSADATA data{};CHECK(WSAStartup(MAKEWORD(2,2),&data)==0);
 struct Socket {SOCKET value;~Socket(){closesocket(value);}} receiver{socket(AF_INET,SOCK_DGRAM,0)};CHECK(receiver.value!=INVALID_SOCKET);
 sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);CHECK(bind(receiver.value,reinterpret_cast<sockaddr*>(&local),sizeof(local))==0);
 int length=sizeof(local);CHECK(getsockname(receiver.value,reinterpret_cast<sockaddr*>(&local),&length)==0);u_long nonblocking=1;CHECK(ioctlsocket(receiver.value,FIONBIO,&nonblocking)==0);
 sockaddr_in sender{};auto read=[&]{std::vector<dx::wsjt::Packet> packets;char bytes[4096];WSAPOLLFD ready{receiver.value,POLLRDNORM,0};while(WSAPoll(&ready,1,10)>0){int size=sizeof(sender);auto count=recvfrom(receiver.value,bytes,sizeof(bytes),0,reinterpret_cast<sockaddr*>(&sender),&size);if(count<0)break;packets.emplace_back(bytes,bytes+count);}return packets;};
 auto folder=std::filesystem::temp_directory_path()/(L"AdvancedDx-UDP-"+std::to_wstring(GetCurrentProcessId()));CHECK(!std::filesystem::exists(folder));std::filesystem::create_directory(folder);
 struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}} cleanup{folder};
 dx::Store store(folder/"history.sqlite3");int64_t now=1800000100;
 dx::Decode d;d.utc=now-60;d.dial=14074000;d.mode="FT8";d.snr=-17;d.message="CQ K1ABC FN42";
 auto insert=[&](dx::Decode row){CHECK(store.insert(row,{39,33},[](const std::string&){return std::make_pair("United States","NA");}));};
 insert(d);auto ft4=d;ft4.mode="FT4";ft4.dial=14080000;insert(ft4);auto replay=d;replay.source="replay";replay.audio=2;insert(replay);
 dx::GridTracker client;dx::GridTrackerContext context;context.channels={{d.dial,d.mode},{ft4.dial,ft4.mode}};context.grid="KM69";
 dx::Json options={{"address","127.0.0.1"},{"port",ntohs(local.sin_port)}};
 client.tick(store,options,context,true,now,10000);CHECK(read().empty()&&!client.state().enabled);
 options["enabled"]=true;client.tick(store,options,context,true,now,10100);auto packets=read();CHECK(client.state().enabled&&client.state().sent==2);
 int decodes=0;for(auto& packet:packets){auto header=dx::wsjt::request(packet.data(),packet.size());CHECK(header);if(header->first==2)++decodes;}CHECK(decodes==2);
 auto request=dx::wsjt::header(7,dx::wsjt::identity({d.dial,d.mode}));CHECK(sendto(receiver.value,reinterpret_cast<const char*>(request.data()),int(request.size()),0,reinterpret_cast<sockaddr*>(&sender),sizeof(sender))==int(request.size()));
 client.tick(store,options,context,true,now,13200);packets=read();CHECK(client.state().lastReply==now);decodes=0;for(auto& packet:packets){auto header=dx::wsjt::request(packet.data(),packet.size());CHECK(header);if(header->first==2)++decodes;}CHECK(decodes==2);
 options["enabled"]=false;client.tick(store,options,context,true,now,14000);CHECK(!client.state().enabled);CHECK(!read().empty());
 std::cout<<"Windows UDP forwarding, default-off, replay requests and recording exclusion passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
