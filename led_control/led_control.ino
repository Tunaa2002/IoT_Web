#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>

const char* ssid = "Not_Name";
const char* password = "muoisokhong";
#define MQTT_SERVER "192.168.2.106"
#define MQTT_PORT 1883
#define MQTT_USER "Tunaa"
#define MQTT_PASSWORD "12345678"
#define MQTT_TEMPERATURE_TOPIC "ESP32/DHT11/Temperature"
#define MQTT_HUMIDITY_TOPIC "ESP32/DHT11/Humidity"
#define MQTT_LED_TOPIC "ESP32/Led"
#define MQTT_LIGHT_TOPIC "ESP32/Light"

#define LED1PIN 2
#define  LED2PIN 18 
#define DHTPIN 4
#define DHTTYPE DHT11

unsigned long previousMillis = 0;
const long interval = 5000;
int current_ledState = LOW;
int last_ledState = LOW;

WiFiClient wificlient;
PubSubClient client(wificlient);
DHT dht(DHTPIN,DHTTYPE);
BH1750 lightMeter(0x23);

void setup_wifi(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  

}

void connect_to_broker(){
  while(!client.connected()){
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe (MQTT_LED_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); 
      Serial.println(" try again in 2 seconds"); 
      delay(2000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.write(payload, length);
  Serial.println();

  String message = String((char*)payload);

  if (strcmp(topic, MQTT_LED_TOPIC) == 0) {
    if (message.indexOf("on led1") != -1) {
      digitalWrite(LED1PIN, HIGH);
    } else if (message.indexOf("off led1") != -1) {
      digitalWrite(LED1PIN, LOW);
    } else if (message.indexOf("on led2") != -1) {
      digitalWrite(LED2PIN, HIGH);
    } else if (message.indexOf("off led2") != -1) {
      digitalWrite(LED2PIN, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);
  Serial.println(F("ESP32-U get data from DHT11 & send to MQTT!")); 
  client.setServer(MQTT_SERVER, MQTT_PORT );
  setup_wifi();
  client.setCallback(callback);
  connect_to_broker();
  dht.begin();
  pinMode(LED1PIN, OUTPUT); 
  digitalWrite(LED1PIN, current_ledState);
  pinMode(LED2PIN, OUTPUT); 
  digitalWrite(LED2PIN, current_ledState);
  Wire.begin();
  lightMeter.begin();
} 

void loop() {
  client.loop();

  if (!client.connected()) {
    connect_to_broker();
  }
  if (last_ledState != current_ledState) { 
    last_ledState = current_ledState;
    digitalWrite(LED1PIN, current_ledState); 
    Serial.print("LED state is "); 
    Serial.println(current_ledState);
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    float h = dht.readHumidity();
    float t = dht.readTemperature(); 
    // Kiểm tra xem có lần đọc nào bị lỗi không và thoát sớm (để thử lại).
    if (isnan(h) || isnan(t)) { 
      Serial.println(F("Failed to read from DHT sensor!")); 
      return;
    } else {
      client.publish(MQTT_TEMPERATURE_TOPIC, String(t).c_str());
      client.publish(MQTT_HUMIDITY_TOPIC, String (h).c_str());
      previousMillis = currentMillis;
      
      uint16_t lux = lightMeter.readLightLevel();
      client.publish(MQTT_LIGHT_TOPIC, String(lux).c_str());

      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.println("%");
      Serial.print(F("Temperature: ")); 
      Serial.print(t);
      Serial.println("ºC"); 
      Serial.print("Light: ");
      Serial.print(lux);
      Serial.println(" Lux");
    }
  }
}