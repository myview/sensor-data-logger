#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Fsm.h"

// Defines
#define XSTR(x) #x
#define STR(x) XSTR(x)

// Used pins
#define LED_PIN     D1
#define BUTTON_PIN  D0
#define SHUTTER_PIN  D0

// Events
#define BUTTON_EVENT        0
#define SUTTER_DOWN_EVENT   1
#define SUTTER_UP_EVENT     2

// Configuration
const char* ssid        = STR(WLAN_SSID);
const char* password    = STR(WLAN_WPA2);
const char* sensor_id   = STR(ID);
const int   interval    = INTERVAL;


#define DELAY_ARRAY_SIZE 10
#define OUTPUT_ARRAY_SIZE 6

int xout[][6]  = {{D5, D1, D2, D4, D3, D6},
                          {D5, D3, D2, D6, D1, D4},
                          {D3, D4, D6, D5, D2, D1},
                          {D2, D3, D5, D4, D6, D1},
                          {D1, D6, D4, D3, D2, D5},
                          {D1, D5, D3, D4, D2, D6}};

const int output_delay[DELAY_ARRAY_SIZE] = {600, 200, 500,  2000, 1000, 100, 500, 1500, 400, 1500};

// Wireless Client
WiFiClient  espClient;

// States
State state_unknown(&unkown, NULL, NULL);
State state_down(&shutter_down, NULL, NULL);
State state_up(&shutter_up, NULL, NULL);

// Final State Machine
Fsm fsm(&state_unknown);

// Set shutter pins with delay
void set_shutter(int pin, int value, int wait)
{
    delay(wait);
    digitalWrite(pin, value);
}

// Set shutter in random order and random delay
void set_shutter_random(int value)
{
    int out = random(0, OUTPUT_ARRAY_SIZE);
    int wait = 0;
    for (int itr = 0; itr < OUTPUT_ARRAY_SIZE; itr ++)
    {
        set_shutter(xout[out][itr], value, wait);
        wait = output_delay[random(0, DELAY_ARRAY_SIZE)];
    }
}

/*
 * Initial State (unkonwn)
 */
void unkown()
{
    Serial.println("state: unkown");
}

/*
 * Shutter Down state
 */
void shutter_down()
{
    Serial.println("shutter down");
    set_shutter_random(LOW);
}

/*
 * Shutter Up state
 */
void shutter_up()
{
    Serial.println("shutter up");
    set_shutter_random(HIGH);
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

    for (int itr = 0; itr < OUTPUT_ARRAY_SIZE; itr ++)
    {
        pinMode(xout[0][itr], OUTPUT);
    }

    pinMode(SHUTTER_PIN, INPUT_PULLUP);

    fsm.add_transition(&state_unknown, &state_down, SUTTER_DOWN_EVENT, NULL);
    fsm.add_transition(&state_unknown, &state_up,   SUTTER_UP_EVENT, NULL);
    fsm.add_transition(&state_down, &state_up, SUTTER_UP_EVENT, NULL);
    fsm.add_transition(&state_up, &state_down, SUTTER_DOWN_EVENT, NULL);

    Serial.print("Connected to: ");
    Serial.println(ssid);
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

    delay(100);
}
