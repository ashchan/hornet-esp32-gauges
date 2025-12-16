# Hornet ESP32 Gauges

ESP32-based gauges for the DCS F/A-18C Hornet.

This project primarily uses the Waveshare ESP32-S3-LCD-1.85 and ESP32-S3-LCD-1.28 round displays to render gauges for the [OpenHornet](https://openhornet.com) project.
* [Waveshare ESP32-S3-LCD-1.85](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.85)
* [Waveshare ESP32-S3-LCD-1.28](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.28)

A dedicated Gateway / Hub ESP32-S3 board communicates with DCS-BIOS, receiving all required aircraft data.
The hub then broadcasts this data in a custom format to multiple gauge ESP32 clients wirelessly using [ESP-NOW](https://www.espressif.com/en/solutions/low-power-solutions/esp-now).

Each client listens only to the data it needs and renders the corresponding gauge on its display.

With this architecture:
* Only the gateway board needs to connect to the DCS PC as a USB serial (COM) device.
* All gauge clients use their USB ports for power only, with no direct PC connection required.

## Credit

The original gauge rendering implementations were created by:
* DCS forum user [Tanarg](https://forum.dcs.world/profile/126894-tanarg/) and [Vinc_Vega](https://forum.dcs.world/profile/13020-vinc_vega/)
* OpenHornet and GitHub community members [shef-code](https://github.com/shef-code) and [scuba](https://github.com/SCUBA82)

The following projects and resources were used as references or starting points:

* IFEI
  - [SCUBA82/OH-IFEI](https://github.com/SCUBA82/OH-IFEI)
  - [ashchan/OH-IFEI-DIS08070H](https://github.com/ashchan/OH-IFEI-DIS08070H) (_My fork for the Elecrow 7" HMI display_)
* Standby Attitude and Roll Indicator Gauge
  - [A-10C Standby ADI](https://forum.dcs.world/topic/281538-code-sketchbook-repository-arduino-unomega-boards-dcs-bios-and-programming-helpplease-post-your-working-sketches-here/#findComment-5190772)
  - [shef-code/F18_SARI_v2](https://github.com/shef-code/F18_SARI_v2)
* Airspeed Indicator
  - [shef-code/F18-AirspeedIndicator](https://github.com/shef-code/F18-AirspeedIndicator)
* Altimeter
  - [shef-code/F18-Altimeter](https://github.com/shef-code/F18-Altimeter)
* Vertical Velocity Indicator
  - [shef-code/F18_Verticle_Velocity_Indicator](https://github.com/shef-code/F18_Verticle_Velocity_Indicator)
* Radar Altimeter Gauge
  - [shef-code/F18-RadarAltimeter](https://github.com/shef-code/F18-RadarAltimeter)
  - [F-18C RALT](https://forum.dcs.world/topic/281538-code-sketchbook-repository-arduino-unomega-boards-dcs-bios-and-programming-helpplease-post-your-working-sketches-here/page/2/#findComment-5402820)
* Battery Gauge
  - [shef-code/F-18_Battery_Gauge](https://github.com/shef-code/F-18_Battery_Gauge)
  - [Producing DCS-BIOS Gauges – Example coding and lessons learned](https://forum.dcs.world/topic/281538-code-sketchbook-repository-arduino-unomega-boards-dcs-bios-and-programming-helpplease-post-your-working-sketches-here/page/2/#findComment-5369279)
* Hydraulic Pressure Gauge
  - [shef-code/F-18_HydraulicPressure_Gauge](https://github.com/shef-code/F-18_HydraulicPressure_Gauge)
  - [Example 2 - Hydraulic Pressure](https://forum.dcs.world/topic/281538-code-sketchbook-repository-arduino-unomega-boards-dcs-bios-and-programming-helpplease-post-your-working-sketches-here/page/2/#findComment-5369295)
* Brake Pressure Gauge
  - [shef-code/F-18_BrakePressure_Gauge](https://github.com/shef-code/F-18_BrakePressure_Gauge)
  - [Example 3 – Brake Pressure Gauge](https://forum.dcs.world/topic/281538-code-sketchbook-repository-arduino-unomega-boards-dcs-bios-and-programming-helpplease-post-your-working-sketches-here/page/2/#findComment-5369303)

## License
This project is licensed under a [Custom Non-Commercial License](LICENSE). You can use and modify it for **personal and educational purposes only**. For commercial use, please contact the authors for permission.