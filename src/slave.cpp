#include "message.hpp"

#include <ESP8266WiFi.h>
#include <espnow.h>


enum SlaveStatus: uint8_t {
    BROADCASTING,
    BEEP,
    PAIRED,
    ENABLED,
    DISABLED
}
status = BROADCASTING;



struct Timer {
    const unsigned long PAIRING_DELAY = 500;
    const unsigned long KEEPALIVE_DELAY = 1000;


    unsigned long last_time = 0;

    bool now_is_time(unsigned long delay) {
        unsigned long elapsed = millis() - last_time;
        return elapsed > delay;
    }

    void update() {
        last_time = millis();
    }
};


Timer timer;
unsigned long pairing_delay = 500;
unsigned long keepalive_delay = 1000;



uint8_t broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};




void blink(uint8_t dt = 200) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(dt);
    digitalWrite(LED_BUILTIN, HIGH);
}


void on_data_recv(uint8_t *mac, uint8_t *incoming_data, uint8_t len) {
    // received from master
    MasterMsg message;
    memcpy(&message, incoming_data, sizeof(message));

    switch (message.type) {
        case MMType::I_PAIRED_YOU:
            status = PAIRED;
            break;

        case MMType::KEEPALIVE:
            status = ENABLED;
            break;
    }
}


void init_wifi() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(on_data_recv);
}



void setup() {
    init_wifi();
}


void try_pairing() {
    SlaveMsg pairing {SMType::PAIR_ME_PLZ};
    esp_now_send(broadcast_address, (uint8_t *) &pairing, sizeof(pairing));
    blink(200);
}



void handle_broadcasting();



void loop() {
    void (*handle_fn)(void);
    unsigned long delay = 0;

    switch (status) {
        case BROADCASTING:
            handle_fn = handle_broadcasting;
            delay = keepalive_delay;
            break;

        default:
            break;
    }

    if (timer.now_is_time(delay)) {
        handle_fn();
        timer.update();
    }
}
