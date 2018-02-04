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

## 2. Reset default firmware on ESP32 (optional)

Press and hold the firmware reset button (some devices)

``` bash
mos flash esp32 --esp-erase-chip
```

## 3. Install to MOS application folder

``` bash
cd ~/.mos/apps.1.25
git clone https://github.com/OffGridNetworks/mos-esp32-wifi-ultrasonic-temp.git
cd mos-esp32-wifi-ultrasonic-temp
mos build --platform esp32 && mos flash
```

## 4. Create an account on Losant

Create a new standalone device and get the device ID

Create an access key and secret using the security menu

Repeat the configuration below whenever the firmware is reset (replace variables or set in environment)

``` bash
mos config-set device.id=LOSANT_DEVICE_ID \
mqtt.client_id=LOSANT_DEVICE_ID \
mqtt.user=LOSANT_CLIENT_ACCESS_KEY \
mqtt.pass=LOSANT_CLIENT_ACCESS_SECRET
```

## 6. Update the WIFI network and password

Create a new standalone device and get the device ID

Create an access key and secret using the security menu (replace variables or set in environment)

``` bash
mos wifi NETWORK_NAME WPA2_NETWORK_PASSWORD
```

## 6. Ensure SSL certs are correct

If this repository hasnt been updated in a while, it's possible the SSL certificates for accessing losant.com in ca.pem are out of date.

Verify with 

``` bash
openssl s_client -host broker.losant.com -port 8883 -prexit -showcerts
```