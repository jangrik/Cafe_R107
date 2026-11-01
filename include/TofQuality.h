#pragma once
#include <stdint.h>

namespace cafe {
// VL53L0X result block at 0x14, matching ST GetRangingMeasurementData:
// status byte, effective SPADs (8.8), signal/ambient rates (9.7), range mm.
// Device code 11 indicates completed ranging. Host-side sigma estimation
// is deliberately not inferred from this raw device status.
struct TofQuality {
  uint8_t statusRaw=0;
  uint16_t spadsRaw=0,signalRaw=0,ambientRaw=0,distanceMm=0;
  static uint16_t word(const uint8_t* p) {return (uint16_t(p[0])<<8)|p[1];}
  static TofQuality decode(const uint8_t* data) {
    TofQuality q;q.statusRaw=data[0];q.spadsRaw=word(data+2);
    q.signalRaw=word(data+6);q.ambientRaw=word(data+8);q.distanceMm=word(data+10);
    return q;
  }
  uint8_t deviceStatus() const {return (statusRaw&0x78)>>3;}
  bool deviceRangeComplete() const {return deviceStatus()==11;}
  float effectiveSpads() const {return spadsRaw/256.0f;}
  float signalRateMcps() const {return signalRaw/128.0f;}
  float ambientRateMcps() const {return ambientRaw/128.0f;}
  const char* deviceStatusName() const {
    switch(deviceStatus()) {
      case 1:case 2:case 3:return "hardware_failure";
      case 4:return "signal_failure";
      case 6:case 9:return "phase_failure";
      case 8:case 10:return "minimum_range_failure";
      case 11:return "range_complete";
      default:return "no_classified_range";
    }
  }
};
}
