#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ------------------------
// WiFi Credentials
// ------------------------
const char* ssid = "Ne";
const char* password = "ikram12345";

// ------------------------
// OpenWeather API Key
// ------------------------
const char* apiKey = "7d7ef394adb071b0b8211c0bae089d2c";

// ------------------------
// MQTT Broker
// ------------------------
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Update every 60 seconds
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000;

// ------------------------
// Connect WiFi
// ------------------------
void connectWiFi() {

  Serial.println();
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ------------------------
// Connect MQTT
// ------------------------
void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Connecting MQTT...");

    String clientId = "ESP32MumbaiWeather-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("Connected");

    } else {

      Serial.print("Failed rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 sec");

      delay(5000);
    }
  }
}

// ------------------------
// Fetch Weather
// ------------------------
void fetchWeather() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Lost");
    return;
  }

  HTTPClient http;

  String url =
    "http://api.openweathermap.org/data/2.5/weather?q=Mumbai&units=metric&appid="
    + String(apiKey);

  Serial.println();
  Serial.println("Fetching Mumbai Weather...");

  http.begin(url);

  int httpCode = http.GET();

  if (httpCode == 200) {

    String payload = http.getString();

    DynamicJsonDocument doc(4096);

    DeserializationError error =
      deserializeJson(doc, payload);

    if (!error) {

      float temp =
        doc["main"]["temp"];

      int humidity =
        doc["main"]["humidity"];

      String condition =
        doc["weather"][0]["description"].as<String>();

      Serial.println("===== MUMBAI WEATHER =====");

      Serial.print("Temperature : ");
      Serial.println(temp);

      Serial.print("Humidity    : ");
      Serial.println(humidity);

      Serial.print("Condition   : ");
      Serial.println(condition);

      // MQTT Publish

      client.publish(
        "weather/mumbai/temp",
        String(temp).c_str(),
        true
      );

      client.publish(
        "weather/mumbai/humidity",
        String(humidity).c_str(),
        true
      );

      client.publish(
        "weather/mumbai/condition",
        condition.c_str(),
        true
      );

      Serial.println("MQTT Publish Success");

    } else {

      Serial.println("JSON Parse Error");
    }

  } else {

    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

// ------------------------
// Setup
// ------------------------
void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("ESP32 Mumbai Weather MQTT");

  connectWiFi();

  client.setServer(
    mqtt_server,
    mqtt_port
  );

  reconnectMQTT();

  fetchWeather();

  lastUpdate = millis();
}

// ------------------------
// Loop
// ------------------------
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  if (millis() - lastUpdate >= updateInterval) {

    fetchWeather();

    lastUpdate = millis();
  }
}