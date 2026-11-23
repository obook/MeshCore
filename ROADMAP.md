# Roadmap

Our MeshCore fork for the ELECROW ThinkNode M5 and M1, branch `thinknode_obook`
of obook/MeshCore. It is the official `dev` branch plus the changes
below, built as the companion Bluetooth firmware
(`ThinkNode_M5_companion_radio_ble`, `ThinkNode_M1_companion_radio_ble`).

## Done

- [x] Adafruit GFX pinned to 1.12.6, as in the official v1.18.0 binary
- [x] The radio's own partition table, so the app image can be flashed
  alone without losing the identity key, contacts and channels
- [x] EU summer time and battery percentage on the screen (PR #3744)
- [x] Time zone UTC+1 by default
- [x] The clock no longer goes back at boot (PR #3741)
- [x] The GPS switch turns the GPS on and off (PR #3748)
- [x] Short sleeps when the phone disconnects (upstream PR #1686, not
  merged yet)
- [x] The sleep checks whether the GPS really runs, not the app setting
- [x] CPU at 80 MHz
- [x] The firmware version carries the git commit (`v1.18.0-<commit>`)
- [x] M1: same clock, time zone, battery percentage and version as the M5
- [x] M1: the sleep cycle no longer cuts a phone that is still pairing
- [x] The node name sits under the date, out of the title bar where the
  battery percentage covered it
- [x] M1: the USB build gets the same screen settings and the GPS
- [x] M6: LEDs and button as in the Elecrow manual, plus a short red flash
  every 3 s on battery (PR #3928)
- [x] M6: the I2C bus is freed at boot, after a reset left the RTC holding
  it (PR #3926)
- [x] M6: 90 s watchdog (PR #3927)
- [x] M6 repeater: a double-click saves the GPS position and sends a
  zero-hop advert, a triple-click turns the GPS on or off

## To check on the hardware

- [ ] Phone disconnected for over a minute, GPS off: the M5 sleeps 12 s,
  wakes 3 s, and MeshVox reconnects
- [ ] Battery life at 80 MHz, against the 8 hours measured on 0558a0e3
  with the GPS off
- [x] Bluetooth, LoRa and the screen still work normally at 80 MHz
- [x] M1 and M5 exchange adverts and direct messages over LoRa
  (v1.18.0-be405ddc). Each radio needs the other's advert before it can
  read its messages
- [ ] M1: pair a phone (PIN 123456) more than a minute after boot
- [ ] M1: clock, time zone and battery percentage on the screen
- [ ] M6: current after a short press on a powered-off board, against a
  normal power off (the radio must sleep in both cases)
- [ ] M6: a short press on battery, in the dark, keeps the board off (the
  charger status pin has no internal pull-up at boot)
- [ ] M6: BLE OTA update with the watchdog running
- [ ] M6: double-click outdoors, with a GPS fix

## To do

- [ ] The M5 never sleeps while the phone stays connected
- [ ] M1: the short sleep cycle saves nothing, since `sleep()` on the
  nRF52 only waits for the next interrupt
- [ ] The light follows the screen again, so the brightness knob has time
  to act (commit bdaf7613, branch `m5-avant-rebase`). Until then:
  `set ui.backlight_delay 15`, and `set ui.backlight_on_keys 1` so both
  side buttons light it
- [ ] Rebase `thinknode_obook` on the official `dev` from time to time
- [ ] Follow PRs #3741, #3744, #3748 and #1686: once they are merged
  upstream, the rebase drops our copies
- [ ] Follow PRs #3926, #3927 and #3928 (M6). In #3928, say that the 3 s
  hold to power on applies to every M6 build, including the low-battery
  power off of the companion; split the PR if reviewers ask
- [ ] M6: the DC/DC converter is never enabled (`begin()` calls
  `NRF52Board::begin()`, not `NRF52BoardDCDC::begin()`). Check first that
  the PCB has the DC/DC inductor, for example in Meshtastic
- [x] M6: from a powered-off board, a double reset may not open the UF2
  drive; plug the USB first. Noted in the M6 notes
- [ ] M6: skip the double-click when no GPS was detected
  (`sensors.isGPSDetected()`)
- [ ] M6: `initiateShutdown()` is declared but never defined, so turning
  on `NRF52_POWER_MANAGEMENT` would fail to link (also upstream)
- [ ] Upstream PR: the M6 double and triple click (board ifdef in the
  repeater main)
- [ ] Follow PR #3929 (GPS on the M1 USB build)

## Flashing the M5

Take a full 4 MB backup, then write only the app image at 0x10000:

    esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 230400 read_flash 0 0x400000 backup.bin
    esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 230400 write_flash 0x10000 .pio/build/ThinkNode_M5_companion_radio_ble/firmware.bin

Do not use `pio run -t upload`, which also rewrites the bootloader and
the boot selection, nor the `-merged.bin` image, which erases the
Bluetooth pairing. Keep the backup out of git: it holds the private key.
At 460800 baud the backup read failed once with "Corrupt data".

## Flashing the M1

On the M1, `pio run -t upload` writes the app over USB and leaves the
bootloader alone:

    pio run -e ThinkNode_M1_companion_radio_ble -t upload --upload-port /dev/ttyACM0

The serial bootloader cannot read the flash back, so there is no backup
step. A double press on reset opens the UF2 drive instead, where you can
copy `firmware.uf2` (built with `-t create_uf2`).
