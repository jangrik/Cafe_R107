#include "Domain.h"
#include "DisplayLogic.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main(){
 setenv("TZ",cafe::TZ_BERLIN,1);tzset();
 assert(!cafe::validTime(0));
 assert(cafe::weekKey(1609459200).empty()); // before supported range
 // 2024-12-30: ISO week belongs to 2025, month remains 2024-12.
 assert(cafe::weekKey(1735556400)=="2025-W01");
 assert(cafe::monthKey(1735556400)=="2024-12");
 // DST spring forward: both instants remain the same local calendar day/week.
 assert(cafe::dayKey(1711845000)=="2024-03-31");
 assert(cafe::dayKey(1711848600)=="2024-03-31");
 // UTC 2024-03-31 22:30 is local Monday 2024-04-01.
 assert(cafe::dayKey(1711924200)=="2024-04-01");
 assert(cafe::weekKey(1711924200)=="2024-W14");
 // Autumn fallback: both occurrences of local 02:30 are on the same date.
 assert(cafe::dayKey(1729989000)=="2024-10-27");
 assert(cafe::dayKey(1729992600)=="2024-10-27");
 assert(cafe::anchoredEpoch(0,10000,1760000000,70000)==1759999940);
 assert(cafe::anchoredEpoch(0,10000,0,70000)==0);
 assert(cafe::anchoredEpoch(1760000001,10000,1760000000,70000)==1760000001);
 cafe::Counts c;c.add(0,1735556400);c.add(1735556400,1735556400);
 assert(c.total==2 && c.undated==1 && c.today==1 && c.week==1 && c.month==1);
 // Clock output is HH:MM only, and absent until a valid time is available.
 assert(cafe::dateKey(0,"%H:%M").empty());
 assert(cafe::dateKey(1711924200,"%H:%M")=="00:30");
 // All-time includes undated coffees; daily/monthly require a valid local date.
 using P=cafe::RankingPeriod;
 assert(cafe::inRankingPeriod(0,0,P::All));
 assert(!cafe::inRankingPeriod(0,1735556400,P::Today));
 assert(!cafe::inRankingPeriod(1735556400,0,P::Month));
 assert(cafe::inRankingPeriod(1735556400,1735556400,P::Today));
 assert(!cafe::inRankingPeriod(1735556400,1735642800,P::Today));
 assert(cafe::inRankingPeriod(1735556400,1735642800,P::Month));
 assert(!cafe::inRankingPeriod(1735556400,1735729200,P::Month));
 // Exactly five, descending counts; ties are deterministic by name then ID.
 auto top=cafe::topFive({{"z","Zero",0},{"b","Bob",3},{"x","X",9},
   {"a2","Anna",3},{"a1","Anna",3},{"d","D",2},{"e","E",1}});
 assert(top.size()==5 && top[0].id=="x" && top[1].id=="a1" &&
        top[2].id=="a2" && top[3].id=="b" && top[4].id=="d");
 assert(cafe::topFive({{"zero","Nobody",0}}).empty());
 assert(cafe::topFive({{"one","One",1}}).size()==1);
 cafe::RankingCycle cycle;cycle.reset(1000);
 assert(!cycle.due(60999));assert(cycle.due(61000));
 assert(cycle.take(61000)==P::Today);
 assert(!cycle.due(66000));assert(!cycle.due(120999));
 assert(cycle.due(121000));assert(cycle.take(121000)==P::Month);
 assert(cycle.take(181000)==P::All);
 assert(cycle.take(241000)==P::Today);
 // An overdue display starts one page, not a burst of missed pages.
 assert(cycle.due(500000));assert(cycle.take(500000)==P::Month);
 assert(!cycle.due(500001));
 // Survive millis() wrapping after ~49 days.
 cycle.reset(UINT32_MAX-1000);
 assert(!cycle.due(58998));assert(cycle.due(58999));
 assert(cycle.take(58999)==P::Today);
 assert(cafe::RANKING_DURATION_MS==5000);
 std::cout<<"Calendar, ISO week year, DST, offline anchors, counts, rankings and display cadence passed.\n";
}
