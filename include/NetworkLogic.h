#pragma once
#include <stdint.h>
namespace cafe {
// Core 2.0.17 scanComplete() times out after 20 * per-channel scan time.
// AP+STA may take longer, so wait for SCAN_DONE before reading the results.
class WifiScanWait {
  uint32_t startedAt_=0;
  bool completed_=false;
public:
  void begin(uint32_t now) {startedAt_=now;completed_=false;}
  void complete() {completed_=true;}
  bool completed() const {return completed_;}
  bool expired(uint32_t now) const {return uint32_t(now-startedAt_)>=15000;}
};
inline bool mayStartWifiScan(bool apOnly,unsigned apClients,bool tried,uint32_t age) {
  // Short scans with a longer retry interval preserve STA recovery with AP clients.
  return !apOnly && (!tried || age>=(apClients?60000U:30000U));
}
}

namespace cafe {
// Apply only after the HTTP response; unsigned elapsed time survives millis wrap.
class NetworkModeChange {
  bool pending_=false,target_=false;
  uint32_t at_=0;
public:
  void schedule(bool apOnly,uint32_t now) {target_=apOnly;at_=now;pending_=true;}
  bool pending() const {return pending_;}
  bool take(uint32_t now,bool& apOnly) {
    if(!pending_ || uint32_t(now-at_)<250)return false;
    apOnly=target_;pending_=false;return true;
  }
};
}
