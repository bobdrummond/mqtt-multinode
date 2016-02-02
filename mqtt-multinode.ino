/**

   @file hass-mqtt_multinode.ino

   @author Bob Drummond
   @date 2016-01-30
*/

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>

#define HOSTNAME_PREFIX "mqttnode-"
#define MQTT_RECONNECT_DELAY 3000

#define BUTTON_TOPIC (node_name + "/button").c_str()

#define ERROR_TOPIC (node_name + "/error").c_str()

#define PAYLOAD_OFF "OFF"
#define PAYLOAD_ON "ON"

#define MOTION_INTERVAL 1000
#define MOTION_TOPIC (node_name + "/motion").c_str()

#define RELAY_CONTROL_TOPIC (node_name + "/relay/set").c_str()
#define RELAY_STATUS_TOPIC (node_name + "/relay").c_str()

#define STATUS_TOPIC (node_name + "/status").c_str()

#define DHT_TYPE DHT22   // DHT 11
#define HUMIDITY_TOPIC (node_name + "/humidity").c_str()
#define TEMPERATURE_TOPIC (node_name + "/temperature").c_str()
#define DHT_FAHRENHEIT true

DHT* dht;

WiFiClient espClient;
PubSubClient client(espClient);

/// @}

/**
   @brief Default WiFi connection information.
   @{
*/
const char* ap_default_psk = "password"; ///< Default PSK.
/// @}



void fatalError() {
  pinMode(BUILTIN_LED, OUTPUT);
  while (true) {
    digitalWrite(BUILTIN_LED, 1);
    delay(100);
    digitalWrite(BUILTIN_LED, 0);
    delay(100);
  }
}
String wifi_psk;
String wifi_ssid;
String mqtt_server;
String mqtt_user;
String mqtt_password;
String node_name;
int dht_pin = -1;
int dht_interval = 10000;
int motion_pin = -1;
int button_pin = -1;
int relay_pin = -1;

bool saveJsonString(JsonObject& json, const char* cfg_name, String* dst) {
  if (json.containsKey(cfg_name)) {
    *dst = String(json[cfg_name].asString());
    Serial.print("Loaded ");
    Serial.print(cfg_name);
    Serial.print(": ");
    Serial.println(*dst);
    return true;
  }
  return false;
}
bool saveJsonInt(JsonObject& json, const char* cfg_name, int* dst) {
  if (json.containsKey(cfg_name)) {
    *dst = json[cfg_name].as<int>();
    Serial.print("Loaded ");
    Serial.print(cfg_name);
    Serial.print(": ");
    Serial.println(*dst);
    return true;
  }
  Serial.print("No ");
  Serial.print(cfg_name);
  Serial.println(" set");
  return false;
}

bool loadJsonConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  Serial.println("config.json contents:");
  Serial.println(buf.get());
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  saveJsonString(json, "wifi_ssid", &wifi_ssid);
  saveJsonString(json, "wifi_psk", &wifi_psk);
  saveJsonString(json, "node_name", &node_name);
  saveJsonString(json, "mqtt_server", &mqtt_server);
  saveJsonString(json, "mqtt_user", &mqtt_user);
  saveJsonString(json, "mqtt_password", &mqtt_password);

  saveJsonInt(json, "dht_pin", &dht_pin);
  saveJsonInt(json, "dht_interval", &dht_interval);
  saveJsonInt(json, "motion_pin", &motion_pin);
  saveJsonInt(json, "button_pin", &button_pin);
  saveJsonInt(json, "relay_pin", &relay_pin);
  return true;
}
void relay_on() {
  Serial.println("turning relay on");
  digitalWrite(relay_pin, 0);
  client.publish(RELAY_STATUS_TOPIC, PAYLOAD_ON, true);
}
void relay_off() {
  Serial.println("turning relay off");
  digitalWrite(relay_pin, 1);
  client.publish(RELAY_STATUS_TOPIC, PAYLOAD_OFF, true);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  byte* p = (byte*)malloc(length + 1);
  p[length] = 0;
  memcpy(p, payload, length);

  String topicstr = String(topic);
  String payloadstr = String((char*)p);

  Serial.println("Received topic:" + topicstr + String(" payload(") + length + "):" + payloadstr);

  if (topicstr == RELAY_CONTROL_TOPIC) {
    if (payloadstr == PAYLOAD_ON) {
      relay_on();
      free(p);
      return;
    }
    else if (payloadstr == PAYLOAD_OFF) {
      relay_off();
      free(p);
      return;
    }
  }

  client.publish(ERROR_TOPIC, p, length);
  free(p);
}

void setup()
{
  Serial.begin(115200);

  delay(100);

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);

  // Set Hostname.
  String hostname(HOSTNAME_PREFIX);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  // Print hostname.
  Serial.println("Hostname: " + hostname);

  // Initialize file system.
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  // Load wifi connection information.
  if (!loadJsonConfig())
  {
    wifi_ssid = "";
    wifi_psk = "";

    Serial.println("Loading config.json failed!");
  }
  else {
    hostname += "-" + node_name;
  }

  WiFi.hostname(hostname);
  if (WiFi.getMode() != WIFI_STA)
  {
    WiFi.mode(WIFI_STA);
    delay(10);
  }

  // ... Compare file config with sdk config.
  if (WiFi.SSID() != wifi_ssid || WiFi.psk() != wifi_psk)
  {
    Serial.println("WiFi config changed.");

    // ... Try to connect to WiFi station.
    WiFi.begin(wifi_ssid.c_str(), wifi_psk.c_str());

    // ... Pritn new SSID
    Serial.print("new SSID: ");
    Serial.println(WiFi.SSID());

    // ... Uncomment this for debugging output.
    //WiFi.printDiag(Serial);
  }
  else
  {
    // ... Begin with sdk config.
    WiFi.begin();
  }

  Serial.println("Wait for WiFi connection.");

  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
  {
    Serial.write('.');
    //Serial.print(WiFi.status());
    delay(500);
  }
  Serial.println();

  // Check connection
  if (WiFi.status() == WL_CONNECTED)
  {
    // ... print IP Address
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Can not connect to WiFi station. Go into AP mode.");

    // Go into software AP mode.
    WiFi.mode(WIFI_AP);

    delay(10);

    Serial.println(String("SSID: ") + hostname.c_str());
    Serial.println(String("Password: ") + ap_default_psk);
    WiFi.softAP(hostname.c_str(), ap_default_psk);

    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
  }

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();

  if (relay_pin >= 0) {
    pinMode(relay_pin, OUTPUT);
    digitalWrite(relay_pin, 1);
  }
  if (motion_pin >= 0) {
    pinMode(motion_pin, INPUT);
  }
  if (dht_pin >= 0) {
    dht = new DHT(dht_pin, DHT_TYPE);
    dht->begin();
  }
  Serial.println(mqtt_server);
  client.setServer(mqtt_server.c_str(), 1883);
  client.setCallback(mqtt_callback);
}

int last_mqtt_reconnect = 0;
bool reconnect() {
  if (!client.connected()) {
    unsigned long currentMillis = millis();
    if (currentMillis < last_mqtt_reconnect + MQTT_RECONNECT_DELAY)
      return false;
    last_mqtt_reconnect = currentMillis;
    Serial.print("Attempting MQTT connection...");
    Serial.print("user: ");
    Serial.print(mqtt_user);
    Serial.print(" password: ");
    Serial.print(mqtt_password);
    if (client.connect(node_name.c_str(), mqtt_user.c_str(), mqtt_password.c_str(), STATUS_TOPIC, 1, 1, "DISCONNECTED")) {
      Serial.println("connected");
      client.publish(STATUS_TOPIC, "CONNECTED", true);
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      return false;
    }
  }
  if (!client.subscribe(RELAY_CONTROL_TOPIC)) {
    Serial.println("failed to subscribe to relay");
    return false;
  }
  return true;
}


bool checkBound(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;
int lastmotion = -1;
int lastbutton = -1;

void check_motion() {
  if (motion_pin == -1) {
    return;
  }
  int motion = digitalRead(motion_pin);
  if (motion != lastmotion) {
    Serial.println(String("motion: ") + motion + " lastmotion:" + lastmotion);
    lastmotion = motion;
    if (motion) {
      client.publish(MOTION_TOPIC, PAYLOAD_ON);
    }
    else {
      client.publish(MOTION_TOPIC, PAYLOAD_OFF, true);
    }
  }
}

void check_button() {
  if (button_pin == -1) {
    return;
  }
  int button = !digitalRead(button_pin);
  if (button != lastbutton) {
    Serial.println(String("button: ") + button + " lastbutton:" + lastbutton);
    lastbutton = button;
    if (button) {
      client.publish(BUTTON_TOPIC, PAYLOAD_ON);
    }
    else {
      client.publish(BUTTON_TOPIC, PAYLOAD_OFF, true);
    }
  }
}

unsigned long dht_last_millis = 0;
char dht_error = 0;
void check_temperature() {
  if (dht_pin == -1) {
    return;
  }
  unsigned long currentMillis = millis();
  if (currentMillis - dht_last_millis < dht_interval) {
    return;
  }
  dht_last_millis = currentMillis;
  float newTemp = dht->readTemperature(DHT_FAHRENHEIT);
  float newHum = dht->readHumidity();
  if (isnan(newTemp) || isnan(newHum)) {
    if (dht_error) return;
    dht_error = 1;
    Serial.println("Failed to read from DHT sensor");
    client.publish(ERROR_TOPIC, "Failed to read from DHT sensor");
    return;
  }
  dht_error = 0;
  if (checkBound(newTemp, temp, diff)) {
    temp = newTemp;
    Serial.println("New temperature:" + String(temp));
    client.publish(TEMPERATURE_TOPIC, String(temp).c_str(), true);
  }

  if (checkBound(newHum, hum, diff)) {
    hum = newHum;
    Serial.println("New humidity:" + String(hum));
    client.publish(HUMIDITY_TOPIC, String(hum).c_str(), true);
  }
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  else {
    check_button();

    long now = millis();
    if (now - lastMsg > 200) {
      lastMsg = now;
      check_motion();
      check_temperature();
    }
  }
  // Handle OTA server.
  ArduinoOTA.handle();
  client.loop();
  yield();
}

