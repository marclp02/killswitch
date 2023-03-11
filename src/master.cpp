#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "message.cpp"

uint8_t slave_address[] = {0xE8, 0xDB, 0x84, 0x99, 0xFE, 0x1D};

struct_keepalive keepalive{};
unsigned long last_time;
unsigned long keepalive_interval = 500;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0){
        Serial.println("Delivery success");
    }
    else{
        Serial.println("Delivery fail");
    }
}

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_add_peer(slave_address, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
    esp_now_register_send_cb(OnDataSent);
}

void loop() {
    unsigned long elapsed = millis() - last_time;
    if (elapsed > keepalive_interval) {
        esp_now_send(slave_address, (uint8_t *) &keepalive, sizeof(keepalive));
    }
}
