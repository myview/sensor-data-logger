#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <WiFiUdp.h>

#define DHTPIN  D4
#define DHTTYPE DHT22
#define XSTR(x) #x
#define STR(x) XSTR(x)

const char* ssid        = STR(WLAN_SSID);
const char* password    = STR(WLAN_WPA2);
const char* mqtt_server = STR(MQTT_IP);
const char* mqtt_topic  = STR(MQTT_TOPIC);
const char* sensor_id   = STR(ID);
const int   mqtt_port   = MQTT_PORT;
const int   interval    = INTERVAL;

DHT             dht(DHTPIN, DHTTYPE);
WiFiClient      espClient;
PubSubClient    client(espClient);

char topic_temperature[150];
char topic_humidity[150];

void setup() {

    Serial.begin(9600);
    Serial.println("...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    strcpy(topic_temperature, mqtt_topic);
    strcat(topic_temperature, sensor_id);
    strcat(topic_temperature, "/temperature");

    strcpy(topic_humidity, mqtt_topic);
    strcat(topic_humidity, sensor_id);
    strcat(topic_humidity, "/humidity");

    print_cfg();
}

void print_cfg() {
    Serial.print("Connected to: ");
    Serial.println(ssid);
    Serial.print("topic 1: ");
    Serial.println(topic_humidity);
    Serial.print("topic 2: ");
    Serial.println(topic_temperature);
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        // Attempt to connect
        if (client.connect("ESP8266Client")) {
            // Serial.print("Attempting MQTT connection...");
            // Serial.println("connected");
            // Once connected, publish an announcement...
            // client.publish("outTopic", "hello world");
            // ... and resubscribe
            // client.subscribe("inTopic");
        } else {
            Serial.print("Attempting MQTT connection...");
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void loop() {

    char msg[50];
    char topic[100];

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))  {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    Serial.print(h);
    Serial.print(" %, ");
    Serial.print(t);
    Serial.print(" *C ");

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))  {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    dtostrf( t, 3, 2, msg );
    client.publish(topic_temperature, msg);

    dtostrf( h, 3, 2, msg );
    client.publish(topic_humidity, msg);  

    Serial.println(" ...");
    delay(interval);
}
