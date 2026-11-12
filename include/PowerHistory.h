#pragma once
#include <stdint.h>
#include <string.h>

namespace cafe {
class PowerHistory {
public:
  static constexpr uint32_t Capacity=3600;
  static constexpr uint8_t Brew=0x80;
  struct Sample {uint16_t wattsPlusOne;uint8_t flags;};

  static uint32_t oldest(uint32_t now) {return now<Capacity?0:now-(Capacity-1);}

  void advance(uint32_t now) {
    if(now-newest_>=Capacity) {
      memset(power_,0,sizeof(power_));
      memset(flags_,0,sizeof(flags_));
    } else {
      while(newest_<now) {
        uint32_t index=++newest_%Capacity;
        power_[index]=0;
        flags_[index]=0;
      }
    }
    newest_=now;
  }

  void record(uint32_t second,uint16_t wattsPlusOne,uint8_t phase) {
    if(second<oldest(newest_) || second>newest_)return;
    uint32_t index=second%Capacity;
    power_[index]=wattsPlusOne;
    flags_[index]=(flags_[index]&Brew)|phase;
  }

  void markBrew(uint32_t second) {
    if(second>=oldest(newest_) && second<=newest_)flags_[second%Capacity]|=Brew;
  }

  Sample get(uint32_t second) const {
    if(second<oldest(newest_) || second>newest_)return {0,0};
    return {power_[second%Capacity],flags_[second%Capacity]};
  }

private:
  uint32_t newest_=0;
  uint16_t power_[Capacity]={};
  uint8_t flags_[Capacity]={};
};
}
