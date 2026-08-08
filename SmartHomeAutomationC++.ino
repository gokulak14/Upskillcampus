#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// Hardware Configuration & Pin Mappings
#define DHTPIN 15
#define DHTTYPE DHT11
#define LDRPIN 34
#define PIRPIN 13

#define RELAY1_PIN 18
#define RELAY2_PIN 19
#define RELAY3_PIN 21
#define RELAY4_PIN 22

// Network & MQTT Broker Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;

// Wi-Fi Connection Setup
void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT Inbound Command Callback
void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(messageTemp);

  if (String(topic) == "home/relay1") {
    digitalWrite(RELAY1_PIN, messageTemp == "ON" ? LOW : HIGH);
  } else if (String(topic) == "home/relay2") {
    digitalWrite(RELAY2_PIN, messageTemp == "ON" ? LOW : HIGH);
  } else if (String(topic) == "home/relay3") {
    digitalWrite(RELAY3_PIN, messageTemp == "ON" ? LOW : HIGH);
  } else if (String(topic) == "home/relay4") {
    digitalWrite(RELAY4_PIN, messageTemp == "ON" ? LOW : HIGH);
  }
}

// MQTT Broker Reconnection Loop
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32SmartHomeClient")) {
      Serial.println("connected");
      client.subscribe("home/relay1");
      client.subscribe("home/relay2");
      client.subscribe("home/relay3");
      client.subscribe("home/relay4");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// System Setup Initialization
void setup() {
  Serial.begin(115200);

  // Initialize Relay Outputs
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Default Relays to OFF state (Active LOW Relays)
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
  digitalWrite(RELAY4_PIN, HIGH);

  // Initialize PIR Input Sensor
  pinMode(PIRPIN, INPUT);

  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Main Execution Loop
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  // Publish telemetry readings every 2000 ms
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int ldrVal = analogRead(LDRPIN);
    int pirState = digitalRead(PIRPIN);

    if (!isnan(t) && !isnan(h)) {
      char tempString[8];
      dtostrf(t, 1, 2, tempString);
      client.publish("home/temperature", tempString);

      char humString[8];
      dtostrf(h, 1, 2, humString);
      client.publish("home/humidity", humString);
    }

    char ldrString[8];
    itoa(ldrVal, ldrString, 10);
    client.publish("home/light", ldrString);

    client.publish("home/motion", pirState == HIGH ? "DETECTED" : "CLEAR");
  }
}