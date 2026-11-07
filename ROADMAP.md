# Roadmap

Our MeshCore fork for the ELECROW ThinkNode M5 and M1, branch `m5_obook`
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

## To do

- [ ] The M5 never sleeps while the phone stays connected
- [ ] M1: the short sleep cycle saves nothing, since `sleep()` on the
  nRF52 only waits for the next interrupt
- [ ] The light follows the screen again, so the brightness knob has time
  to act (commit bdaf7613, branch `m5-avant-rebase`). Until then:
  `set ui.backlight_delay 15`, and `set ui.backlight_on_keys 1` so both
  side buttons light it
- [ ] Rebase `m5_obook` on the official `dev` from time to time
- [ ] Follow PRs #3741, #3744, #3748 and #1686: once they are merged
  upstream, the rebase drops our copies

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
