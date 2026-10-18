#include "Domain.h"
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
 std::cout<<"Calendar, ISO week year, DST, offline anchors and counts passed.\n";
}
