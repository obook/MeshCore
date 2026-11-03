# Roadmap

Our MeshCore fork for the ELECROW ThinkNode M5, branch `m5_obook` of
obook/MeshCore. It is the official `dev` branch plus the changes below,
built as the companion Bluetooth firmware
(`ThinkNode_M5_companion_radio_ble`).

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

## To check on the hardware

- [ ] Phone disconnected for over a minute, GPS off: the M5 sleeps 12 s,
  wakes 3 s, and MeshVox reconnects
- [ ] Battery life at 80 MHz, against the 8 hours measured on bfb5ac29
  with the GPS off
- [ ] Bluetooth, LoRa and the screen still work normally at 80 MHz

## To do

- [ ] The M5 never sleeps while the phone stays connected
- [ ] Rebase `m5_obook` on the official `dev` from time to time
- [ ] Follow PRs #3741, #3744, #3748 and #1686: once they are merged
  upstream, the rebase drops our copies

## Flashing

Take a full 4 MB backup, then write only the app image at 0x10000:

    esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 460800 read_flash 0 0x400000 backup.bin
    esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 460800 write_flash 0x10000 .pio/build/ThinkNode_M5_companion_radio_ble/firmware.bin

Do not use `pio run -t upload`, which also rewrites the bootloader and
the boot selection, nor the `-merged.bin` image, which erases the
Bluetooth pairing. Keep the backup out of git: it holds the private key.
