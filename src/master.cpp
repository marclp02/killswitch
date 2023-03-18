#include "message.hpp"
#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool paired = false;

unsigned long lastTime = 0;
unsigned long pairingDelay = 500;
unsigned long keepaliveDelay = 200;

Message pairingMessage{RECEIVE_PAIRING};
Message keepAliveMessage{KEEP_ALIVE};

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    Message message;
    memcpy(&message, incomingData, sizeof(message));
//    Message *message = (Message *) incomingData;

    switch (message.mType) {
        case RECEIVE_PAIRING:
            break;
        case SEND_PAIRING:
            for (int i = 0; i < 6; i++) {
                Serial.print(mac[i], HEX);
                if (i < 5) {
                    Serial.print(":");
                }
            }
            paired = true;
            esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0);
            esp_now_send(nullptr, (uint8_t *) &pairingMessage, sizeof(pairingMessage));
        case BEEP:
        case KEEP_ALIVE:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    unsigned long elapsed = millis() - lastTime;
    if (paired && elapsed > keepaliveDelay) {
        esp_now_send(nullptr, (uint8_t *) &keepAliveMessage, sizeof(keepAliveMessage));
        lastTime = millis();
    }
}
