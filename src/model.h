// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
namespace dx {
using Json=nlohmann::json;
struct Point {double lat=0,lon=0;};
std::optional<Point> gridPoint(const std::string& grid);
std::pair<double,double> distanceBearing(Point from,Point to);
std::vector<Point> greatCircle(Point from,Point to,int steps=128);
Point sunPosition(int64_t utc);
std::string upper(std::string s);
std::string countryName(std::string name);
std::string callsignLookupUrl(const std::string& call,int provider);
bool matches(const std::string& value,const std::string& patterns);
struct Meaning {std::string transmitter,called,grid,type="Text";};
Meaning meaning(const std::string& message);
enum class MessageKind {All,GeneralCall,Exchange,Other};
MessageKind messageKind(const Meaning& identity);
struct ActivityBearing {std::optional<double> degrees;double alignment=0;int stations=0;};
struct Decode;
struct Row;
ActivityBearing activityBearing(const std::vector<Decode>& decodes);
std::vector<Decode> farthestActivity(Point home,const std::vector<Row>& rows,int count);
std::string compassDirection(double degrees);
float arrivalOpacity(double seconds);
inline constexpr double arrivalHighlightSeconds=9;
std::string latestHeardSummary(const Decode& decode);
uint32_t callHash(const std::string& call,int bits);
struct Decode {
 int64_t utc=0,dial=0,slotMs=0; int audio=0,snr=0;
 std::optional<double> dt; std::string mode,message,raw,source="live",country,continent;
 Meaning identity; std::optional<Point> position; std::optional<double> km,bearing;
 std::string gridSource; int64_t gridUtc=0; bool firstCall=false,firstGrid=false,firstCountry=false,distanceRecord=false;
 std::string identitySource="literal";
 Json json() const; static Decode fromJson(const Json& j);
 std::string key() const;
};
struct Filter {
 std::string search,call,called,prefix,country,continent,grid,band,mode,frequency,text,type;
 std::string excludeCall,excludeCalled,excludeCountry,excludeGrid;
 bool cqOnly=false,hasGrid=false,first=false;
 MessageKind messageKind=MessageKind::All;
 double minKm=0,maxKm=40080,minSnr=-100,maxDt=30;
 bool accepts(const Decode& d) const;
 Json json() const; static Filter fromJson(const Json& j);
};
struct Query {int64_t since=0,until=INT64_MAX,afterRowId=0,firstSince=0,firstAfterId=0;Filter filter;int sort=0;bool descending=true;bool messages=false,uniqueCalls=false;int pathCount=3,offset=0;};
struct Row {Decode decode;int64_t count=1,firstHeardUtc=0,stationFirstHeardUtc=0;};
struct ExchangePath{std::string transmitter,called;int64_t utc=0,fromGridUtc=0,toGridUtc=0;std::vector<Point> points;};
struct Stats {int64_t countries=0,stations=0,recentStations=0,messages=0;double farthest=0,activeSeconds=0;std::string farthestCountry,farthestCall;std::vector<Decode> latest;std::vector<std::string> heardCountries,firstCountryKeys;};
std::string bandName(int64_t hz);
struct Job {uint64_t id=0;int64_t utcMs=0,dial=0,deadlineMs=0;std::string mode,source="live";std::vector<int16_t> pcm;int retry=0;};
struct Preset {int64_t dial;std::string mode;};
std::vector<Preset> presets();
std::vector<Preset> eligible(double center,double width,const std::vector<Preset>& all);
}
