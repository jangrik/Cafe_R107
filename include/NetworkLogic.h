#pragma once
#include <stdint.h>
namespace cafe {
inline bool mayStartWifiScan(bool apOnly,unsigned apClients,bool tried,uint32_t age) {
  return !apOnly && apClients==0 && (!tried || age>=30000);
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
