#pragma once
#include <stdint.h>

namespace cafe {
constexpr uint32_t HEALTH_INTERVAL_MS=60000;
constexpr uint32_t TOF_STALL_MS=5000;
class HealthSchedule {
  uint32_t checkedAt_=0;
public:
  void reset(uint32_t now) {checkedAt_=now;}
  bool takeIfDue(uint32_t now) {
    if(uint32_t(now-checkedAt_)<HEALTH_INTERVAL_MS)return false;
    checkedAt_=now;return true;
  }
};
// Invalid distances still constitute completed measurements. They do not
// appear here: only transport/init or an absent measurement stream warrants recovery.
inline const char* tofHealthError(bool initialized,bool ack,bool sampled,
    uint32_t sampleAge,uint32_t initAge) {
  if(!ack)return "i2c_not_responding";
  if(!initialized)return "not_initialized";
  if((sampled?sampleAge:initAge)>=TOF_STALL_MS)return "measurement_stalled";
  return "";
}
inline bool readerVersionPlausible(uint8_t value) {return value!=0 && value!=0xff;}
inline bool readerProbeOK(uint8_t expected,const uint8_t* values,unsigned count) {
  if(!readerVersionPlausible(expected) || !count)return false;
  unsigned matches=0;
  for(unsigned i=0;i<count;++i)if(values[i]==expected)++matches;
  return matches>=count/2+1;
}
}
