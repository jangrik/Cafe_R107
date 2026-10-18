#pragma once
#include "Domain.h"
#include <algorithm>
#include <vector>

namespace cafe {
enum class RankingPeriod : uint8_t { Today, Month, All };
constexpr uint32_t RANKING_INTERVAL_MS=60000;
constexpr uint32_t RANKING_DURATION_MS=5000;

// Starts are 60 s apart (55 s normal display + 5 s ranking). Unsigned
// subtraction handles millis() rollover. A busy display defers, never queues,
// an overdue page; bookings take priority over the ranking.
class RankingCycle {
  uint32_t lastStart_=0;
  RankingPeriod next_=RankingPeriod::Today;
public:
  void reset(uint32_t now) { lastStart_=now; next_=RankingPeriod::Today; }
  bool due(uint32_t now) const { return uint32_t(now-lastStart_)>=RANKING_INTERVAL_MS; }
  RankingPeriod take(uint32_t now) {
    RankingPeriod result=next_;
    next_=result==RankingPeriod::Today ? RankingPeriod::Month :
          result==RankingPeriod::Month ? RankingPeriod::All : RankingPeriod::Today;
    lastStart_=now;
    return result;
  }
};

inline bool inRankingPeriod(int64_t timestamp,int64_t now,RankingPeriod period) {
  if(period==RankingPeriod::All) return true; // Includes explicitly undated coffees.
  if(!validTime(timestamp) || !validTime(now)) return false;
  return period==RankingPeriod::Today ? dayKey(timestamp)==dayKey(now) :
                                       monthKey(timestamp)==monthKey(now);
}

struct RankEntry { std::string id, name; uint32_t count; };
inline std::vector<RankEntry> topFive(std::vector<RankEntry> entries) {
  entries.erase(std::remove_if(entries.begin(),entries.end(),
      [](const RankEntry& row){return row.count==0;}),entries.end());
  std::sort(entries.begin(),entries.end(),[](const RankEntry& a,const RankEntry& b){
    if(a.count!=b.count) return a.count>b.count;
    if(a.name!=b.name) return a.name<b.name;
    return a.id<b.id;
  });
  if(entries.size()>5) entries.resize(5);
  return entries;
}
}
