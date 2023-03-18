#include <ESP8266WiFi.h>
#include <espnow.h>
#include "message.hpp"

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool paired = false;
bool enabled = false;

unsigned long lastTime = 0;
unsigned long pairingDelay = 500;
unsigned long keepaliveDelay = 500;

Message pairingMessage{SEND_PAIRING};

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    Message message = *(Message *) incomingData;
    switch (message.mType) {
        case RECEIVE_PAIRING:
            paired = true;
            break;
        case SEND_PAIRING:
        case BEEP:
            break;
        case KEEP_ALIVE:
            digitalWrite(LED_BUILTIN, LOW);
            enabled = true;
            lastTime = millis();
            break;

    }
}

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    unsigned long elapsed = millis() - lastTime;
    if (!paired && elapsed > pairingDelay) {
        esp_now_send(broadcastAddress, (uint8_t *) &pairingMessage, sizeof(pairingMessage));
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        lastTime = millis();
    }
    if (paired && elapsed > keepaliveDelay) {
        enabled = false;
        digitalWrite(LED_BUILTIN, HIGH);
        lastTime = millis();
    }
}
