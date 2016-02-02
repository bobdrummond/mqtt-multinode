# MQTT MultiNode
## Overview
This is meant to be a home automation node. It runs on an ESP8266 and uses MQTT to talk to a central server. I'm working with Home Assistant (https://home-assistant.io) at the moment.

I think this is in a rough, early stage, but it's doing useful things for me already and I was asked to share it.

## Setup
Components:
- HC-SR501 PIR Motion Sensor
  - Setup: an integer value for "motion_pin" in the config.json. Delete the key to disable.
  - MQTT output on "<node_name>/motion". "ON" when <motion_pin> transitions high, "OFF" when it transitions low
- DHT22 Temperature and Humidity Sensor
  - Setup: an integer value for "dht_pin" in the config.json. Delete the key to disable.
  - MQTT output on "<node_name>/temperature" and "<node_name>/humidity" respectively
  - Temperature is in Fahrenheit, but can be changed with the #define IS_FAHRENHEIT in the code
- Relay
  - Setup: an integer value for "relay_pin" in the config.json. Delete the key to disable.
  - I'm not sure exactly what cheap ebay relay module I have, but it works with a 3.3 supply. It enables when the "IN" pin is pulled low
  - MQTT input expected on "<node_name>/relay/set". "ON" will pull <relay_pin> low, "OFF" will set it high
  - MQTT output on "<node_name>/relay" echos the state after it is set
- Button
  - Setup: an integer value for "button_pin" in the config.json. Delete the key to disable.
  - MQTT output on "<node_name>/button". "ON" when <button_pin> transitions low, "OFF" when it transitions high
Other:
- Status
  - MQTT output on "<node_name>/status" of "ON" when the node sucessfully connects the the MQTT server. A "last will and testament" is set to send "OFF" on the same topic if the node is disconnected
- Error
  - MQTT output on "<node_name>/error"
  - If the node receives a message on a subscribed topic(currently only "<node_name>/relay/set") and it cannot parse it, it will echo the message on the error topic
  - If the node fails to read from the DHT22 sensor, it will publish "Failed to read from DHT sensor" on the error topic

![Fritzing Breadboard](/schematic/mqtt-multinode_bb.jpg?raw=true "Fritzing Breadboard")

## Installation
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