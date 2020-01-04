

#include "Fsm.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <WEMOS_SHT3X.h>


// Defines
#define XSTR(x) #x
#define STR(x) XSTR(x)
#define WORKER_OFF 0
#define WORKER_ON 1
#define WORKER_TIMEOUT 0


// Used pins
#define SHUTTER_PIN                 D0

// Events
#define BUTTON_EVENT                0
#define SUTTER_DOWN_EVENT           1
#define SUTTER_UP_EVENT             2
#define SUTTER_AUTO_EVENT           3
#define SUTTER_DOWN_FORCE_EVENT     4
#define SUTTER_UP_FORCE_EVENT       5

// Configuration
const char* ssid        = STR(WLAN_SSID);
const char* password    = STR(WLAN_WPA2);
const char* sensor_id   = STR(ID);
const int   interval    = INTERVAL;


#define DELAY_ARRAY_SIZE 10
#define OUTPUT_ARRAY_SIZE 6

int worker[][OUTPUT_ARRAY_SIZE] = \
                  { {WORKER_OFF, LOW, WORKER_TIMEOUT, D4},
                    {WORKER_OFF, LOW, WORKER_TIMEOUT, D3},
                    {WORKER_OFF, LOW, WORKER_TIMEOUT, D7},
                    {WORKER_OFF, LOW, WORKER_TIMEOUT, D8},
                    {WORKER_OFF, LOW, WORKER_TIMEOUT, D5},
                    {WORKER_OFF, LOW, WORKER_TIMEOUT, D6}
                  };

// Wireless Client
WiFiClient  espClient;

SHT3X       sht30(0x45);

// States
State state_unknown(&unkown, NULL, NULL);
State state_down(&shutter_down_random, NULL, NULL);
State state_up(&shutter_up_random, NULL, NULL);
State state_down_forced(&shutter_down, NULL, NULL);
State state_up_forced(&shutter_up, NULL, NULL);

// Final State Machine
Fsm fsm(&state_unknown);

// Web Server on port 80
AsyncWebServer server(80);

// Set shutter in random order and random delay
void set_shutter_random(int value, int rand) {
    for (int i = 0; i < OUTPUT_ARRAY_SIZE; i++) {
        worker[i][0] = WORKER_ON;
        worker[i][1] = value;
        if (rand == 1) {
            worker[i][2] = random(0, 10);
        } else {
            worker[i][2] = 0;
        }
    }
}

/*
 * Initial State (unkonwn)
 */
void unkown() {
    Serial.println("state: unkown");
}

/*
 * Shutter Down state
 */
void shutter_down()
{
    Serial.println("shutter down");
    set_shutter_random(LOW, 0);
}

/*
 * Shutter Up state
 */
void shutter_up()
{
    Serial.println("shutter up");
    set_shutter_random(HIGH, 0);
}

/*
 * Shutter Down state
 */
void shutter_down_random()
{
    Serial.println("shutter down");
    set_shutter_random(LOW, 1);
}

/*
 * Shutter Up state
 */
void shutter_up_random()
{
    Serial.println("shutter up");
    set_shutter_random(HIGH, 1);
}


void buttonClicked(const char* id) {
    Serial.println("Button Clicked - " + String(id));
    if (strcmp(id, "auto") == 0){
       fsm.trigger(SUTTER_AUTO_EVENT);
    }
    if (strcmp(id, "up") == 0){
       fsm.trigger(SUTTER_UP_FORCE_EVENT);
    }
    if (strcmp(id, "down") == 0){
       fsm.trigger(SUTTER_DOWN_FORCE_EVENT);
    }
}



/*
 * Device Setup
 */
void setup()
{
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    for (int i = 0; i < OUTPUT_ARRAY_SIZE; i++) {
        pinMode(worker[i][3], OUTPUT);
    }

    pinMode(SHUTTER_PIN, INPUT_PULLUP);

    /*
     * ----------------------------------------------------------------------
     * STATE TRANSITIONS
     */

    // Auto
    fsm.add_transition(&state_unknown, &state_down, SUTTER_DOWN_EVENT, NULL);
    fsm.add_transition(&state_unknown, &state_up,   SUTTER_UP_EVENT, NULL);
    fsm.add_transition(&state_down, &state_up, SUTTER_UP_EVENT, NULL);
    fsm.add_transition(&state_up, &state_down, SUTTER_DOWN_EVENT, NULL);

    // Forced
    fsm.add_transition(&state_down, &state_up_forced, SUTTER_UP_FORCE_EVENT, NULL);
    fsm.add_transition(&state_up, &state_down_forced, SUTTER_DOWN_FORCE_EVENT, NULL);
    fsm.add_transition(&state_down_forced, &state_up_forced, SUTTER_UP_FORCE_EVENT, NULL);
    fsm.add_transition(&state_up_forced, &state_down_forced, SUTTER_DOWN_FORCE_EVENT, NULL);
    fsm.add_transition(&state_down_forced, &state_unknown, SUTTER_AUTO_EVENT, NULL);
    fsm.add_transition(&state_up_forced, &state_unknown, SUTTER_AUTO_EVENT, NULL);

    Serial.print("Connected to: ");
    Serial.println(ssid);

    ESPDash.init(server);   // Initiate ESPDash and attach your Async webserver instance
    ESPDash.addTemperatureCard("temp1", "Temperature Card", 0, 20);
    ESPDash.addHumidityCard("hum1", "Humidity Card", 98);

    ESPDash.addButtonCard("auto", "Auto");
    ESPDash.addButtonCard("down", "Down");
    ESPDash.addButtonCard("up", "Up");
    ESPDash.attachButtonClick(buttonClicked);

    server.begin();
}

/*
 * Main Loop
 */
void loop() {

    int buttonState = digitalRead(SHUTTER_PIN);
    if (buttonState == LOW) {
        fsm.trigger(SUTTER_DOWN_EVENT);
    } else {
        fsm.trigger(SUTTER_UP_EVENT);
    }

    fsm.run_machine();

    if(sht30.get()==0){
        ESPDash.updateHumidityCard("hum1",sht30.humidity);
        ESPDash.updateTemperatureCard("temp1", sht30.cTemp);
    }

    for (int i = 0; i < OUTPUT_ARRAY_SIZE; i++) {
        if (worker[i][0] == WORKER_ON) {
            if (worker[i][2] <=  WORKER_TIMEOUT) {
                digitalWrite(worker[i][3], worker[i][1]);
            } else {
                worker[i][2]--;
            }
        }
    }

    delay(1000);
}
