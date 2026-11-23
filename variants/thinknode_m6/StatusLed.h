#pragma once

#include <stdint.h>

#define M6_BOOT_LED_MILLIS   10000   // Elecrow: red LED on for 10 s at power-on
#define M6_BATT_FULL_MV      4080    // 100 % on Meshtastic's M6 battery curve
#define M6_BATT_CRITICAL_MV  3500    // about 5 %, Meshtastic's critical level
#define M6_HEARTBEAT_MILLIS  3000    // Meshtastic flashes every second, which looks too busy

// Red LED pattern of the Meshtastic M6 firmware (StatusLEDModule): on while
// booting, slow blink while charging, steady once charged, 1 ms flash every
// 3 s on battery, fast blinks for 2 s every 30 s when the battery is critical.
// Returns the new LED state; wait_ms is the delay before the next call.
inline bool m6RedLed(bool on, uint32_t now, bool charging, uint16_t batt_mv, uint32_t& wait_ms) {
  if (now < M6_BOOT_LED_MILLIS) {
    wait_ms = M6_BOOT_LED_MILLIS - now;
    return true;
  }

  if (charging) {
    wait_ms = 1000;
    if (batt_mv >= M6_BATT_FULL_MV) {
      return true;
    }
    return !on;
  }

  bool critical = batt_mv > 1000 && batt_mv <= M6_BATT_CRITICAL_MV;   // below 1 V is a bad reading
  if (critical) {
    wait_ms = 250;
    bool in_burst = (now % 30000) < 2000;
    if (in_burst) {
      return !on;
    }
    return false;
  }

  if (on) {
    wait_ms = M6_HEARTBEAT_MILLIS - 1;
  } else {
    wait_ms = 1;
  }
  return !on;
}
