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
