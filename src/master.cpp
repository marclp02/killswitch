#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
//#include "SSD1306Ascii.h"
//#include "SSD1306AsciiWire.h"
#include "message.cpp"

#define I2C_ADDRESS 0x3C
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


struct_pairing pairing;
struct_keepalive keepalive;

bool paired = false;
unsigned long last_time = 0;
unsigned long keepalive_interval = 250;  // .5-sec between packets

//SSD1306AsciiWire oled;

void receive_pairing(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    memcpy(&pairing, incomingData, sizeof(pairing));
    esp_now_add_peer(pairing.mac, ESP_NOW_ROLE_SLAVE, 1, nullptr, 0);
    esp_now_unregister_recv_cb();
    Serial.print("paired");
}

void send_keepalive() {
    esp_now_send(nullptr, (uint8_t *) &pairing, sizeof(pairing));
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_MAX);
    esp_now_register_recv_cb(receive_pairing);

    //WiFi.macAddress(pairing_out.mac);
    //pairing_out.channel = 0;

    //keepalive.keepalive = 1;

    //Wire.begin(2,0);
    //Wire.setClock(400000L);
    //oled.begin(&Adafruit128x64, I2C_ADDRESS);
    //oled.setFont(System5x7);
    //oled.clear();
    //oled.print(" OK ");

}

void loop() {
    unsigned long elapsed = millis() - last_time;
    if (paired && elapsed > keepalive_interval) {
        send_keepalive();
        last_time = millis();
    }
    yield();
}