# MQTT MultiNode
## Overview
This is meant to be a home automation node. It runs on an ESP8266 and uses MQTT to talk to a central server. I'm working with Home Assistant (https://home-assistant.io) at the moment.

I think this is in a rough, early stage, but it's doing useful things for me already and I was asked to share it.

## Use
1. Install the required libraries. You can find them in the Arduino library manager.
  - DHT sensor library by Adafruit Version 1.2.3 (Not Unified)
  - ArduinoJson by Benoit Blanchon Version 5.0.7
  - PubSubClient by Nick O'Leary Version 2.4.0
2. Create a config.json file and place it in the "data" directory. You can use config-example.json as a reference. Pin numbers vary by platform. The Sparkfun ESP8266 Thing uses the ESP8266 numbers on the silkscreen. The NodeMCU has a different mapping you can find here: https://github.com/nodemcu/nodemcu-devkit-v1.0
3. Make sure you have ESP8266FS (https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#flash-layout) and use it to upload your config.json.
4. Upload the sketch.
5. Verify it's working with the serial monitor

## Known bugs/future plans
It doesn't handle config.json errors as well as I'd like. I'm planning to play around with OTA updates more, as well as an HTTP server to modify the config. I'm also planning on adding some kind of status LED support, and a rotary encoder for changing lights in the room.