#ifdef MASTER
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "message.cpp"

// Receiver MAC address
uint8_t slave[] = {0xE8, 0xDB, 0x84, 0x99, 0xFE, 0x1D};

unsigned long last_time = 0;
unsigned long keepalive_interval = 500;  // 1-sec between packets

// Success or failure
void on_data_sent(uint8_t *mac_addr, uint8_t sendStatus) {
    if (sendStatus == 0){
        // Success
    }
    else{
        // Failure
    }
}

void setup() {
    WiFi.mode(WIFI_STA);
    esp_now_init();

    // Set as Master
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

    // Callback when data is sent
    //esp_now_register_send_cb(on_data_sent);

    // Add Slave
    esp_now_add_peer(slave, ESP_NOW_ROLE_SLAVE, 1, nullptr, 0);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    if ((millis() - last_time) > keepalive_interval) {
        digitalWrite(LED_BUILTIN, HIGH);
        message packet{};
        packet.keepalive = true;

        // Send packet to Slave
        esp_now_send(slave, (uint8_t *) &packet, sizeof(packet));


        //
        last_time = millis();
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }
}
#endif