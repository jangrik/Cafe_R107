#pragma once
#include <stdint.h>
namespace cafe {
inline bool mayStartWifiScan(bool apOnly,unsigned apClients,bool tried,uint32_t age) {
  return !apOnly && apClients==0 && (!tried || age>=30000);
}
}
