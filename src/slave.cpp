#include <ESP8266WiFi.h>
#include <espnow.h>
#include "message.cpp"

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct_pairing pairing;
struct_keepalive keepalive;

bool paired = false;
unsigned long last_time = 0;
unsigned long keepalive_interval = 500;
unsigned long pairing_interval = 200;


void receive_keepalive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    paired = true;
    memcpy(&keepalive, incomingData, sizeof(keepalive));
    if (keepalive.keepalive == 1) {
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
    }
    last_time = millis();
}

void send_pairing() {
    digitalWrite(LED_BUILTIN, LOW);
    esp_now_send(broadcastAddress, (uint8_t *) &pairing, sizeof(pairing));
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_MAX);
    WiFi.macAddress(pairing.mac);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    esp_now_register_recv_cb(receive_keepalive);

}

void loop() {
    unsigned long elapsed = millis() - last_time;
    if (!paired && elapsed > pairing_interval) {
        send_pairing();
        last_time = millis();
    } else if (paired && elapsed > keepalive_interval) {
        if (!keepalive.keepalive) {
            digitalWrite(LED_BUILTIN, HIGH);
        }
        last_time = millis();
    }
    yield();
}