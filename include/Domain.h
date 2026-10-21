#pragma once
#include <stdint.h>
#include <time.h>
#include <string>

namespace cafe {
constexpr const char* VERSION = "1.0.15";
constexpr const char* TZ_BERLIN = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr int64_t MIN_EPOCH = 1704067200; // 2024-01-01 UTC
constexpr int64_t MAX_EPOCH = 2145916800LL; // 2038-01-01 UTC (core 2 uses 32-bit time_t)
inline bool validTime(int64_t t) { return t >= MIN_EPOCH && t < MAX_EPOCH; }

// Input uses epoch seconds; calendar operations use the configured Berlin TZ.
inline std::string dateKey(int64_t epoch, const char* format) {
  if(!validTime(epoch)) return "";
  time_t t=static_cast<time_t>(epoch); struct tm local{};
  localtime_r(&t,&local); char b[32]; strftime(b,sizeof(b),format,&local);
  return b;
}
inline std::string weekKey(int64_t t) { return dateKey(t,"%G-W%V"); }
inline std::string monthKey(int64_t t) { return dateKey(t,"%Y-%m"); }
inline std::string dayKey(int64_t t) { return dateKey(t,"%Y-%m-%d"); }

inline int64_t anchoredEpoch(int64_t epoch, uint64_t uptime,
                             int64_t anchorEpoch, uint64_t anchorUptime) {
  if(validTime(epoch)) return epoch;
  if(!validTime(anchorEpoch)) return 0;
  int64_t result=anchorEpoch+(static_cast<int64_t>(uptime)-static_cast<int64_t>(anchorUptime))/1000;
  return validTime(result) ? result : 0;
}

struct Counts {
  uint32_t total=0, today=0, week=0, month=0, undated=0;
  void add(int64_t t, int64_t now) {
    ++total;
    if(!validTime(t)) { ++undated; return; }
    if(!validTime(now)) return;
    if(dayKey(t)==dayKey(now)) ++today;
    if(weekKey(t)==weekKey(now)) ++week;
    if(monthKey(t)==monthKey(now)) ++month;
  }
};
}


