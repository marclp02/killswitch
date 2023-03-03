#ifdef SLAVE

#include <ESP8266WiFi.h>
#include <espnow.h>
#include "message.cpp"

bool keep_alive = false;
unsigned long last_time = 0;
unsigned long keepalive_interval = 1000;  // 1-sec between packets

void on_receive_data_callback(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    message packet;
    memcpy(&packet, incomingData, sizeof(packet));
    if (packet.keepalive) {
        keep_alive = true;
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println(1);
    } else {
        keep_alive = false;
        Serial.println(0);

    }
    last_time = millis();
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(on_receive_data_callback);
    //pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    if ((millis() - last_time) > keepalive_interval) {
        keep_alive = false;
        //digitalWrite(LED_BUILTIN, LOW);
        Serial.println(0);
        last_time = millis();
    } else {
        //digitalWrite(LED_BUILTIN, HIGH);
    }
}

#endif