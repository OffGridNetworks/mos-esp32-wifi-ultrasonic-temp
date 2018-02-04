## Overview

This is a Mongoose OS app for the ESP32 Heltec WifiKit or ESP32 Heltec Lora with OLED.   It should work with other IOT boards connected to a SSD1306 over I2C by changing the `mos.yml` config settings.

It removes the default adafruit logo and allows dynamic bitmap loading via Javascript (just
create a Base64 string with [image converter](https://www.espruino.com/Image+Converter)).

It measures a distance using a HC-SR04 Ultrasonic, adjusting for the density of the air using a DS18B20 wire temperature, and both displays and publishes the readings to MQTT (Losant).

It is used in "production" as a sump pit level to provide alerts if the sump pump is not working.

## 1. Install MOS

``` bash
curl -fsSL https://mongoose-os.com/downloads/mos/install.sh | /bin/bash
~/.mos/bin/mos --help      
~/.mos/bin/mos
```

## 2. Install to MOS application folder

``` bash
cd ~/.mos/apps.1.21
git clone https://github.com/OffGridNetworks/mos-esp32-wifi-ultrasonic-temp.git
cd mos-esp32-wifi-ultrasonic-temp
mos build --platform esp32 && mos flash
```

