#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Button.h>
#include <ButtonEventCallback.h>
#include <Adafruit_MPR121.h>
#include <Wire.h>

const int PIN_TOUCH_IRQ = 4;
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

char mqtt_server[40] = "...";
char mqtt_port[6] = "1883";
char mqtt_topic_prefix[40] = "/touch/";

Adafruit_MPR121 touchSensor = Adafruit_MPR121();
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  wifiManager.setAPCallback(configModeCallback);

  if(!wifiManager.autoConnect("mqtt-button", "raspberry")) {
    Serial.println("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }

  Serial.println("connected...yeey :)");
  if (!touchSensor.begin()) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  client.setServer(mqtt_server, atoi(mqtt_port));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  currtouched = touchSensor.touched();
  for (uint8_t i=0; i<12; i++) {
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" touched");
      String topic = mqtt_topic_prefix;
      topic += String(ESP.getChipId(), HEX) + "/" + String(i, DEC);
      client.publish(topic.c_str(), "touched");
    }
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" released");
      String topic = mqtt_topic_prefix;
      topic += String(ESP.getChipId(), HEX) + "/" + String(i, DEC);
      client.publish(topic.c_str(), "released");
    }
  }
  lasttouched = currtouched;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "mqtt-buttons-";
    clientId += String(ESP.getChipId(), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
