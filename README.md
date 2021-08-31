# Open Bike Computer RTOS

## Summary
This repo is an open source bike computer based on freeRTOS on ~~ESP8266~~ ESP32. (ESP32 is much newer hardware, no reason to stick with esp8266 right now)
This is an attempt of learning C and freeRTOS env. I've first made bike computer using arduino nano, so this might be refered as version 2. I've also tackled a little of 3d design in blender to create a case for this bike computer.

Creating community around this project for all programmers riding their bikes would be awesome :)


## Features
### Core:
- current speed, distance, time
- average, max speed, total distance

### Memory:
- average/max speed and total distance are stored on the device (not synchronized for now, only local)

### Synchronization using WiFi:
- synchronize ride data with OBC Server (see https://github.com/Random90/OBC_Server)
- wifi settings are hardcoded at compile-time
- data is sent the the obc_server on ride stop as a new ride after 30 seconds of inactivity (configurable at compile-time)
- ride is restarted automaticly after ride synchonization succeded
- sntp time synchronization - sets timestamp for your rides 
### Current supported hardware:
- ESP32
- Reed Switch
- pcd8544 Nokia 5110 LCD

### 3D Printable case
See /3dcase folder for files. Print both "obc_case_bottom" and obc_case_top" .stl files.

### Additional funcionality TODOS and ideas:
- better sleep mode for battery performance
- local ride storage for later sync (on new ride started by button press) (spiffs)
- full ride logs with file rotation
- wifi access point setup, multiple WIFI access point settings
- timezone settings for time display
- modularization of components (start with screen selection at compile-time)
- different hardware support e.g different type of screens like OLED, E-ink, color LCD
- buttons
- automatic screen brightness
- live, reed impulse online sync (share your ride data realtime)
- sync with Strava API, Wifi geolocation.
- Even later in future: GPS module, GPRS module for live tracking. 


## Hardware

### Required components (3d case is prepared specifically for that modules):
- ESP_WROOM_32 DEVKIT V1 
- Nokia 5110 LCD HXE
- Reed switch

### Default wiring
OBC uses VSPI (can be HSPI, but I use it for debug) to connect to LCD:
| ESP32 | LCD |
--- | --- |
| D4 | RST |
| D5 | CE |
| D25 | DC |
| D19 | DIN |
| D18 | CLK |
| 3V3/5V | VIN |
| RX2/G16 | BL |
| GND | GND |

ReedSwitch -> ESP D21 PIN

JTAG CJMCU-2232HL (note to myself)

| ESP32 | JTAG |
--- | --- |
| D13 | AD0 |
| D12 | AD1 |
| D15 | AD2 |
| D14 | AD3 |
| GND | GND |