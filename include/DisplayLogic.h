#pragma once
#include "Domain.h"
#include <algorithm>
#include <vector>

namespace cafe {
enum class RankingPeriod : uint8_t { Today, Month, All };
constexpr uint32_t RANKING_PAGE_MS=10000;
constexpr uint16_t RANKING_HAND_MM=120;
constexpr uint32_t RANKING_RELEASE_MS=700;
inline bool rankingHand(bool freshValid,uint16_t mm) {
  return freshValid && mm>=20 && mm<RANKING_HAND_MM;
}
class RankingCycle {
  uint32_t pageAt_=0,lastNearAt_=0;
  bool active_=false;
  RankingPeriod period_=RankingPeriod::Today;
public:
  void reset(uint32_t now) {pageAt_=now;lastNearAt_=now;active_=false;period_=RankingPeriod::Today;}
  bool active() const {return active_;}
  RankingPeriod period() const {return period_;}
  bool update(bool hand,uint32_t now) {
    if(hand)lastNearAt_=now;
    // Keep both the page and its timer through short sensor/loop gaps.
    if(!hand && (!active_ || uint32_t(now-lastNearAt_)>=RANKING_RELEASE_MS)) {
      bool changed=active_;reset(now);return changed;
    }
    if(!active_) {active_=true;period_=RankingPeriod::Today;pageAt_=now;return true;}
    if(uint32_t(now-pageAt_)<RANKING_PAGE_MS)return false;
    period_=period_==RankingPeriod::Today?RankingPeriod::Month:
        period_==RankingPeriod::Month?RankingPeriod::All:RankingPeriod::Today;
    pageAt_=now;return true;
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

