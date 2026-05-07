# Gateway / Hub

The Gateway / Hub runs on the Waveshare ESP32-S3-Zero with 4MB flash and 2MB PSRAM.
Use the `hub` PlatformIO environment for this board.

## Board Environments

Use the `hub` PlatformIO environment for the Waveshare ESP32-S3-Zero.
If using a normal/larger ESP32-S3 board, upload with the `hub-s3` environment instead.

## Uploading

The ESP32-S3-Zero uses native USB instead of a separate USB-UART bridge. If upload fails with:

```text
Failed to connect to ESP32-S3: No serial data received.
```

put the board into ROM bootloader mode manually:

1. Hold `BOOT`.
2. Tap and release `RESET` while still holding `BOOT`.
3. Release `BOOT`.
4. Run the PlatformIO upload again.

Close DCS-BIOS, serial monitors, or any other program using the hub serial port before uploading.

## Runtime Serial

The `hub` environment enables USB CDC on boot so DCS-BIOS can connect to the board as a USB serial device.

## ESP-NOW Signal Strength

If one or more gauges do not update reliably from the ESP32-S3-Zero hub, especially when the same gauges work with a larger ESP32-S3 hub board, increase the hub's ESP-NOW transmit power in the `hub` environment:

```ini
[env:hub]
build_flags =
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DESP_MAX_TX_POWER=60
```

`esp_wifi_set_max_tx_power()` uses 0.25 dBm units, and values are rounded into supported power levels:

| Value | TX power |
| ---: | ---: |
| 8 | 2 dBm |
| 20 | 5 dBm |
| 28 | 7 dBm |
| 34 | 8.5 dBm |
| 44 | 11 dBm |
| 52 | 13 dBm |
| 56 | 14 dBm |
| 60 | 15 dBm |
| 66 | 16.5 dBm |
| 72 | 18 dBm |
| 80 | 20 dBm |

Start with `60` and increase only if needed.
