# Standby Compass

## Uploading

The display board uses native USB instead of a separate USB-UART bridge. If upload fails with:

```text
Failed to connect to ESP32-S3: No serial data received.
```

put the board into ROM bootloader mode manually:

1. Hold `BOOT`.
2. Tap and release `RESET` while still holding `BOOT`.
3. Release `BOOT`.
4. Run the PlatformIO upload again.

Close DCS-BIOS, serial monitors, or any other program using the hub serial port before uploading.

After a successful upload, the ESP32-S3-LCD-1.47B may stay blank even though PlatformIO reports a hard reset. Press `RESET` once to start the newly flashed firmware. This appears to be a board reset behavior after flashing, not a display rendering failure.
