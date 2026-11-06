#pragma once
#include <stdint.h>

namespace cafe {
constexpr uint64_t SHELLY_GRACE_MS=10000;
constexpr uint64_t SHELLY_ALARM_MS=30000;
constexpr uint64_t TOKEN_RESERVATION_MS=120000;
inline bool shellyHeartbeatFresh(bool seen,bool healthy,uint64_t age) {
  return seen && healthy && age<=15000;
}
inline bool liveBrewAge(int64_t age) {
  return age>=0 && age<=int64_t(SHELLY_GRACE_MS);
}
inline bool fallbackMatches(int64_t tokenMinusEnd) {
  return tokenMinusEnd>=-int64_t(TOKEN_RESERVATION_MS) &&
      tokenMinusEnd<=int64_t(SHELLY_GRACE_MS);
}
inline bool brewAlarm(uint64_t age) {
  return age>SHELLY_GRACE_MS && age<=SHELLY_GRACE_MS+SHELLY_ALARM_MS;
}
}
