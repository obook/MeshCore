#pragma once

#include <stdint.h>

// LED behaviour from the Elecrow ThinkNode M6 manual.
#define M6_BOOT_LED_MILLIS        10000   // red on for 10 s at power-on
#define M6_BATT_LOW_MV            3500    // low-battery alert at 3.5 V or less
#define M6_LOW_BATT_PERIOD_MILLIS 5000    // slow flashing at 0.2 Hz
#define M6_LOW_BATT_FLASH_MILLIS  500     // our choice: the manual gives no duty cycle
#define M6_DATA_BLINK_MILLIS      100     // rapid flashing at 5 Hz: 100 ms on, 100 ms off
#define M6_HEARTBEAT_MILLIS       3000    // not in the manual: a short sign of life on battery
#define M6_HEARTBEAT_FLASH_MILLIS 2
#define M6_SOLAR_BEAT_MILLIS      1000    // not in the manual: faster while the sun charges

// Red PWR LED: on for 10 s at power-on, steady on USB, a 2 ms flash every
// second while the sun charges, slow flashing when the battery is low,
// otherwise a 2 ms flash every 3 s. The manual's steady light while the sun
// charges stayed on for seconds at a time under changing light.
inline bool m6RedLed(uint32_t now, bool usb, bool solar, uint16_t batt_mv) {
  if (now < M6_BOOT_LED_MILLIS) {
    return true;
  }
  if (usb) {
    return true;
  }
  if (solar) {
    return now % M6_SOLAR_BEAT_MILLIS < M6_HEARTBEAT_FLASH_MILLIS;
  }
  bool low_battery = batt_mv > 1000 && batt_mv <= M6_BATT_LOW_MV;   // below 1 V is a bad reading
  if (low_battery) {
    return now % M6_LOW_BATT_PERIOD_MILLIS < M6_LOW_BATT_FLASH_MILLIS;
  }
  return now % M6_HEARTBEAT_MILLIS < M6_HEARTBEAT_FLASH_MILLIS;
}

// Blue DATA LED: rapid flashing while LoRa data is sent or received,
// starting with a flash.
inline bool m6BlueLed(bool data_active, uint32_t data_elapsed_ms) {
  if (!data_active) {
    return false;
  }
  return (data_elapsed_ms / M6_DATA_BLINK_MILLIS) % 2 == 0;
}
