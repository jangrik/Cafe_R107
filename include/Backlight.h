#pragma once
#include <stdint.h>
#include <cstring>

namespace cafe {
constexpr uint32_t BACKLIGHT_IDLE_MS=10UL*60UL*1000UL;
constexpr uint32_t PROXIMITY_WAKE_MS=30000;
constexpr uint16_t PRESENCE_NEAR_MM=800, PRESENCE_FAR_MM=1100;

// Proximity grants a short look at the display; only chip contacts reset the
// ten-minute booking window. Continuous presence cannot keep it on forever.
class BacklightTimer {
  enum class Mode { Auto, On };
  Mode mode_=Mode::Auto;
  uint32_t chipAt_=0, proximityAt_=0;
  bool chipWindow_=true, proximityWindow_=false;
public:
  void reset(uint32_t now) { chipAt_=now;chipWindow_=true;proximityWindow_=false;mode_=Mode::Auto; }
  bool setMode(const char* mode) {
    if(!mode)return false;
    if(std::strcmp(mode,"auto")==0)mode_=Mode::Auto;
    else if(std::strcmp(mode,"on")==0)mode_=Mode::On;
    else if(std::strcmp(mode,"off")==0) {
      // Behave exactly like expired inactivity timers; wake sources stay armed.
      chipWindow_=false;proximityWindow_=false;mode_=Mode::Auto;
    }
    else return false;
    return true;
  }
  const char* modeName() const {return mode_==Mode::On?"on":"auto";}
  void chip(uint32_t now) { chipAt_=now;chipWindow_=true; }
  void approach(uint32_t now) { proximityAt_=now;proximityWindow_=true; }
  bool on(uint32_t now) {
    if(chipWindow_ && uint32_t(now-chipAt_)>=BACKLIGHT_IDLE_MS)chipWindow_=false;
    if(proximityWindow_ && uint32_t(now-proximityAt_)>=PROXIMITY_WAKE_MS)proximityWindow_=false;
    return mode_==Mode::On || (mode_==Mode::Auto && (chipWindow_ || proximityWindow_));
  }
};

class RestartRequest {
  bool pending_=false;uint32_t requestedAt_=0;
public:
  static constexpr uint32_t DELAY_MS=500;
  void schedule(uint32_t now) {if(!pending_){pending_=true;requestedAt_=now;}}
  bool due(uint32_t now) const {return pending_ && uint32_t(now-requestedAt_)>=DELAY_MS;}
  uint32_t remaining(uint32_t now) const {
    if(!pending_ || due(now))return 0;
    return DELAY_MS-uint32_t(now-requestedAt_);
  }
};
inline bool freshTofSample(bool available,bool sampled,bool valid,uint32_t ageMs) {
  return available && sampled && valid && ageMs<=500;
}

// Three near samples wake; five far/invalid samples rearm. Hysteresis avoids
// repeated triggers when a person moves around the 80 cm threshold.
class PresenceGate {
  bool present_=false;
  uint8_t near_=0, far_=0;
public:
  bool update(bool valid,uint16_t mm) {
    if(valid && mm<=PRESENCE_NEAR_MM) {
      far_=0;
      if(near_<3)++near_;
      if(near_==3 && !present_) {present_=true;return true;}
    } else if(!valid || mm>=PRESENCE_FAR_MM) {
      near_=0;
      if(far_<5)++far_;
      if(far_==5)present_=false;
    } else {near_=0;far_=0;}
    return false;
  }
  bool present() const {return present_;}
};
}
