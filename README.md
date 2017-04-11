# MQTT MultiNode

I found the project [ESPEasy](https://www.letscontrolit.com/wiki/index.php/ESPEasy), and started using that instead of continuing development on this.

## Overview
This is meant to be a home automation node. It compiles with the Arduino IDE, runs on an ESP8266 and uses MQTT to talk to a central server. I'm working with Home Assistant (https://home-assistant.io) at the moment.

I think this is in a rough, early stage, but it's doing useful things for me already and I was asked to share it.

## Setup
###Components:
- HC-SR501 PIR Motion Sensor
  - Setup: an integer value for "motion_pin" in the config.json. Delete the key to disable.
  - MQTT output on **_node_name_/motion**. "ON" when _motion_pin_ transitions high, "OFF" when it transitions low
- DHT22 Temperature and Humidity Sensor
  - Setup: an integer value for "dht_pin" in the config.json. Delete the key to disable.
  - MQTT output on **_node_name_/temperature** and **_node_name_/humidity** respectively
  - Temperature is in Fahrenheit, but can be changed with the #define IS_FAHRENHEIT in the code
- Relay
  - Setup: an integer value for "relay_pin" in the config.json. Delete the key to disable.
  - I'm not sure exactly what cheap ebay relay module I have, but it works with a 3.3 supply. It enables when the "IN" pin is pulled low
  - MQTT input expected on **_node_name_/relay/set**. "ON" will pull _relay_pin_ low, "OFF" will set it high
  - MQTT output on **_node_name_/relay** echos the state after it is set
- Button
  - Setup: an integer value for "button_pin" in the config.json. Delete the key to disable.
  - MQTT output on **_node_name_/button**. "ON" when _button_pin_ transitions low, "OFF" when it transitions high

###Other:
- Status
  - MQTT output on **_node_name_/status** of "ON" when the node sucessfully connects the the MQTT server. A "last will and testament" is set to send "OFF" on the same topic if the node is disconnected
- Error
  - MQTT output on **_node_name_/error**
  - If the node receives a message on a subscribed topic(currently only **_node_name_/relay/set**) and it cannot parse it, it will echo the message on the error topic
  - If the node fails to read from the DHT22 sensor, it will publish "Failed to read from DHT sensor" on the error topic

![Fritzing Breadboard](/schematic/mqtt-multinode_bb.jpg?raw=true "Fritzing Breadboard")

## Installation
An MQTT broker is required to do anything useful with this sketch. Most of the [References](#references) have details on setting up an MQTT broker. I'm using [Mosquitto](http://mosquitto.org/)  and I'm happy with it.

1. Install the required libraries. You can find them in the Arduino library manager.
  - DHT sensor library by Adafruit Version 1.2.3 (Not Unified)
  - ArduinoJson by Benoit Blanchon Version 5.0.7
  - PubSubClient by Nick O'Leary Version 2.4.0
2. Create a config.json file and place it in the "data" directory. You can use config-example.json as a reference. Pin numbers vary by platform. The Sparkfun ESP8266 Thing uses the ESP8266 numbers on the silkscreen. The NodeMCU has a different mapping you can find here: https://github.com/nodemcu/nodemcu-devkit-v1.0
3. Make sure you have ESP8266FS (https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#flash-layout) and use it to upload your config.json.
4. Upload the sketch.
5. Verify it's working with the serial monitor

## Known bugs
It doesn't handle config.json errors as well as I'd like. Be careful with the _Flash Size_ setting, especially if you have several different platforms. The Arduino IDE resets all of the board settings to defaults when you change board type, so it's very easy to do. If you switch the size or SPIFFS size, either the config will get trashed or the code will look at the wrong address. It's a minor annoyance if you have to go grab the thing from another room/floor to reflash it with a USB cable.

## Future Plans
- Improve OTA updates
- Add HTTP web interface to modify the config.
- Status LED
- Rotary encoder for changing lights in the room

## References
The code in this repo is mostly a combination of the following sources. I'd like to thank them for some awesome guides.
- https://home-assistant.io/blog/2015/10/11/measure-temperature-with-esp8266-and-report-to-mqtt/
- http://www.instructables.com/id/An-inexpensive-IoT-enabler-using-ESP8266/
- Arduino IDE examples for ArduinoOTA, ESP8266mDNS, DHT, ArduinoJson, and PubSubClient
