#include "Domain.h"
#include "TofQuality.h"
#include "NetworkLogic.h"
#include "CoffeeLogic.h"
#include "PowerHistory.h"
#include "DisplayLogic.h"
#include "Backlight.h"
#include "HealthLogic.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main(){
 cafe::PowerHistory powerHistory;
 powerHistory.advance(4);
 powerHistory.markBrew(2);
 powerHistory.record(2,1401,4);
 powerHistory.record(4,1,1);
 assert(powerHistory.get(2).wattsPlusOne==1401 && powerHistory.get(2).flags==(4|cafe::PowerHistory::Brew));
 assert(powerHistory.get(3).flags==0 && powerHistory.get(4).wattsPlusOne==1);
 powerHistory.advance(3601);
 assert(powerHistory.get(2).flags!=0);
 powerHistory.advance(3602);
 powerHistory.record(2,30,1);
 assert(powerHistory.get(2).flags==0 && powerHistory.get(3602).flags==0);
 powerHistory.markBrew(3602);
 powerHistory.advance(7202);
 assert(powerHistory.get(7202).flags==0);

 // Decode the real ST result-block layout, including fractional rates/SPADs.
 const uint8_t resultBlock[12]={0x5f,0,0x12,0x80,0,0,0x01,0x40,0x00,0x20,0x01,0x2c};
 auto quality=cafe::TofQuality::decode(resultBlock);
 assert(quality.deviceStatus()==11 && quality.deviceRangeComplete());
 assert(quality.distanceMm==300 && quality.effectiveSpads()==18.5f);
 assert(quality.signalRateMcps()==2.5f && quality.ambientRateMcps()==0.25f);
 quality.statusRaw=4<<3;assert(!quality.deviceRangeComplete());
 assert(std::string(quality.deviceStatusName())=="signal_failure");
 quality.statusRaw=9<<3;assert(std::string(quality.deviceStatusName())=="phase_failure");
 quality.statusRaw=0;assert(std::string(quality.deviceStatusName())=="no_classified_range");

 cafe::NetworkModeChange modeChange;bool target=false;
 assert(!modeChange.take(1000,target));modeChange.schedule(true,1000);
 assert(modeChange.pending());assert(!modeChange.take(1249,target));
 assert(modeChange.take(1250,target) && target && !modeChange.pending());
 assert(!modeChange.take(1500,target));
 modeChange.schedule(true,2000);modeChange.schedule(false,2100);
 assert(!modeChange.take(2349,target));assert(modeChange.take(2350,target) && !target);
 modeChange.schedule(true,UINT32_MAX-100);assert(!modeChange.take(148,target));
 assert(modeChange.take(149,target) && target);

 // Reproduce the real AP+STA log: Core's 2400 ms timeout precedes
 // the driver's SCAN_DONE at 4382 ms. Results must not be read early.
 cafe::WifiScanWait scanWait;scanWait.begin(34034);
 assert(!scanWait.completed() && !scanWait.expired(36439));
 assert(!scanWait.completed() && !scanWait.expired(38416));
 scanWait.complete();assert(scanWait.completed());
 // A new scan must not inherit the old completion. Missing callbacks have
 // a bounded watchdog, including across millis() rollover.
 scanWait.begin(66441);assert(!scanWait.completed());
 assert(!scanWait.expired(81440));assert(scanWait.expired(81441));
 scanWait.begin(UINT32_MAX-1000);
 assert(!scanWait.completed() && !scanWait.expired(13998));
 assert(scanWait.expired(13999));

 assert(!cafe::mayStartWifiScan(true,0,false,60000));
 assert(cafe::mayStartWifiScan(false,1,false,0));
 assert(!cafe::mayStartWifiScan(false,1,true,59999));
 assert(cafe::mayStartWifiScan(false,1,true,60000));
 assert(cafe::mayStartWifiScan(false,0,false,0));
 assert(!cafe::mayStartWifiScan(false,0,true,29999));
 assert(cafe::mayStartWifiScan(false,0,true,30000));
 const uint32_t scanAt=UINT32_MAX-1000,scanNow=28999;
 assert(cafe::mayStartWifiScan(false,0,true,uint32_t(scanNow-scanAt)));

 cafe::ReaderFailures failures;
 for(unsigned i=0;i<9;++i) {failures.observe(true);assert(!failures.repeated());}
 failures.observe(false);assert(failures.count()==0); // no-card resets the streak
 for(unsigned i=0;i<10;++i)failures.observe(true);
 assert(failures.repeated());failures.observe(false);assert(!failures.repeated());

 cafe::TofRecovery recovery;
 assert(!recovery.pending());assert(!recovery.measurement());
 assert(!recovery.begin()); // first attempt: software
 assert(recovery.pending());assert(recovery.begin()); // retry: hardware
 assert(recovery.begin()); // failed retries keep using hardware
 assert(recovery.measurement()); // exactly one success for the first new sample
 assert(!recovery.measurement());assert(!recovery.pending());
 assert(!recovery.begin()); // a later incident starts with software again
 assert(recovery.measurement());
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
 assert(!cafe::rankingHand(true,120));assert(cafe::rankingHand(true,119));
 assert(!cafe::rankingHand(false,50));assert(!cafe::rankingHand(true,0));
 cafe::RankingCycle cycle;cycle.reset(1000);
 assert(!cycle.update(false,61000) && !cycle.active());
 const uint32_t page=cafe::RANKING_PAGE_MS,begin=62000;
 assert(cycle.update(true,begin) && cycle.period()==P::Today);
 assert(!cycle.update(true,begin+page-1));
 assert(cycle.update(true,begin+page) && cycle.period()==P::Month);
 assert(cycle.update(true,begin+2*page) && cycle.period()==P::All);
 assert(cycle.update(true,begin+3*page) && cycle.period()==P::Today);
 assert(!cycle.update(false,begin+3*page+699) && cycle.active());
 assert(cycle.update(false,begin+3*page+700) && !cycle.active());
 cycle.reset(UINT32_MAX-1000);assert(cycle.update(true,UINT32_MAX-1000));
 assert(!cycle.update(true,page-1002));
 assert(cycle.update(true,page-1001) && cycle.period()==P::Month);
 // A brief dropout at a page transition preserves the gesture and page timer.
 cycle.reset(0);assert(cycle.update(true,0));
 assert(!cycle.update(true,page-100));
 assert(cycle.update(false,page) && cycle.period()==P::Month);
 assert(!cycle.update(true,page+100) && cycle.period()==P::Month);
 assert(!cycle.update(true,2*page-100));
 assert(cycle.update(false,2*page) && cycle.period()==P::All);
 cycle.reset(UINT32_MAX-100);assert(cycle.update(true,UINT32_MAX-100));
 assert(!cycle.update(false,598));
 assert(cycle.update(false,599) && !cycle.active());
 // Lighting: exact ten-minute cutoff and configured proximity grant.
 cafe::BacklightTimer light;light.reset(1000);
 assert(light.on(600999));assert(!light.on(601000));
 light.approach(602000);assert(light.on(602000));
 assert(light.on(602000+cafe::PROXIMITY_WAKE_MS-1));assert(!light.on(602000+cafe::PROXIMITY_WAKE_MS));
 light.chip(640000);assert(light.on(1239999));assert(!light.on(1240000));
 // Proximity cannot shorten the existing chip window.
 light.chip(2000000);light.approach(2001000);
 assert(light.on(2031000));assert(light.on(2599999));assert(!light.on(2600000));
 light.reset(UINT32_MAX-1000);
 assert(light.on(598998));assert(!light.on(598999));
 light.approach(UINT32_MAX-1000);
 assert(light.on(cafe::PROXIMITY_WAKE_MS-1002));assert(!light.on(cafe::PROXIMITY_WAKE_MS-1001));
 // Expired windows do not reappear on the next millis() wrap.
 assert(!light.on(UINT32_MAX-900));
 cafe::PresenceGate gate;
 assert(!gate.update(true,700));assert(!gate.update(true,700));
 assert(gate.update(true,700));assert(gate.present());
 for(int i=0;i<100;i++)assert(!gate.update(true,600)); // Holding still never retriggers.
 for(int i=0;i<5;i++)assert(!gate.update(true,900)); // Hysteresis retains presence.
 assert(gate.present());
 for(int i=0;i<5;i++)assert(!gate.update(true,1200));
 assert(!gate.present());
 assert(!gate.update(true,500));assert(!gate.update(true,500));assert(gate.update(true,500));
 for(int i=0;i<5;i++)assert(!gate.update(false,0));
 assert(!gate.present());
 // A sporadic near reading is not enough; an intervening far reading resets it.
 assert(!gate.update(true,700));assert(!gate.update(true,1200));
 assert(!gate.update(true,700));assert(!gate.update(true,700));assert(gate.update(true,700));
 // API off expires both timers and returns to auto, rather than blocking wakeups.
 light.reset(0);light.approach(1);
 assert(light.setMode("off"));assert(!light.on(2));
 assert(std::strcmp(light.modeName(),"auto")==0);
 assert(!light.setMode("invalid"));assert(!light.setMode(nullptr));assert(!light.on(3));
 assert(light.setMode("auto"));assert(!light.on(4)); // No old timer resurrected.
 light.chip(10);assert(light.on(10));assert(light.on(600009));assert(!light.on(600010));
 light.setMode("off");light.approach(700000);assert(light.on(700000));
 assert(light.on(700000+cafe::PROXIMITY_WAKE_MS-1));assert(!light.on(700000+cafe::PROXIMITY_WAKE_MS));
 // Off also exits forced-on, and a following RFID contact wakes normally.
 light.setMode("on");assert(light.on(900000));
 light.setMode("off");assert(!light.on(900000));
 light.chip(900001);assert(light.on(900001));
 // Forced-on and reset semantics remain unchanged.
 light.setMode("on");assert(light.on(1600000));
 light.setMode("auto");assert(!light.on(1600000));
 light.reset(1700000);assert(light.on(1700000));
 assert(std::strcmp(light.modeName(),"auto")==0);
 assert(cafe::freshTofSample(true,true,true,500));
 assert(!cafe::freshTofSample(true,true,true,501));
 assert(!cafe::freshTofSample(false,true,true,0));
 assert(!cafe::freshTofSample(true,false,true,0));
 assert(!cafe::freshTofSample(true,true,false,0));
 cafe::RestartRequest restart;assert(!restart.due(1000));assert(restart.remaining(1000)==0);
 restart.schedule(UINT32_MAX-200);assert(restart.remaining(UINT32_MAX-200)==500);
 restart.schedule(UINT32_MAX-100); // Repeated requests do not postpone the reboot.
 assert(!restart.due(298));assert(restart.remaining(298)==1);
 assert(restart.due(299));assert(restart.remaining(299)==0);
 // The supervisor runs once a minute, with no retry storm or millis wrap bug.
 cafe::HealthSchedule supervisor;supervisor.reset(1000);
 assert(!supervisor.takeIfDue(60999));assert(supervisor.takeIfDue(61000));
 assert(!supervisor.takeIfDue(61001));assert(supervisor.takeIfDue(121000));
 supervisor.reset(UINT32_MAX-1000);
 assert(!supervisor.takeIfDue(58998));assert(supervisor.takeIfDue(58999));
 assert(std::strcmp(cafe::tofHealthError(true,true,true,100,50000),"")==0);
 // Completed but invalid range values still refresh measurement age, so no reset.
 assert(std::strcmp(cafe::tofHealthError(true,true,true,100,900000),"")==0);
 assert(std::strcmp(cafe::tofHealthError(true,true,true,5000,50000),"measurement_stalled")==0);
 assert(std::strcmp(cafe::tofHealthError(true,false,true,100,50000),"i2c_not_responding")==0);
 assert(std::strcmp(cafe::tofHealthError(false,true,false,0,50000),"not_initialized")==0);
 assert(std::strcmp(cafe::tofHealthError(true,true,false,0,5000),"measurement_stalled")==0);
 uint8_t goodProbes[]={0x92,0x92,0x92},oneGlitch[]={0xff,0x92,0x92},badProbes[]={0xff,0x92,0xff};
 assert(cafe::readerProbeOK(0x92,goodProbes,3));
 assert(cafe::readerProbeOK(0x92,oneGlitch,3));
 assert(!cafe::readerProbeOK(0x92,badProbes,3));
 assert(!cafe::readerVersionPlausible(0));assert(!cafe::readerVersionPlausible(0xff));
 // Per-chip metadata can use the same persistent boot anchor, even after a reset.
 assert(cafe::anchoredEpoch(0,5000,1760000000,65000)==1759999940);
 assert(!cafe::shellyHeartbeatFresh(false,true,0));
 assert(!cafe::shellyHeartbeatFresh(true,false,0));
 assert(cafe::shellyHeartbeatFresh(true,true,15000));
 assert(!cafe::shellyHeartbeatFresh(true,true,15001));
 assert(!cafe::liveBrewAge(-1));assert(cafe::liveBrewAge(10000));
 assert(!cafe::liveBrewAge(10001));
 assert(!cafe::brewAlarm(10000));assert(cafe::brewAlarm(10001));
 assert(cafe::brewAlarm(40000));assert(!cafe::brewAlarm(40001));
 assert(cafe::fallbackMatches(-120000));assert(cafe::fallbackMatches(10000));
 assert(!cafe::fallbackMatches(-120001));assert(!cafe::fallbackMatches(10001));
 std::cout<<"Calendar, ISO week year, DST, offline anchors, counts, rankings, display cadence and backlight/presence passed.\n";
}


