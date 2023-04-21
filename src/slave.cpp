#include "utils.hpp"

#include <ESP8266WiFi.h>
#include <espnow.h>


#define BROADCAST_DELAY   100
#define KEEPALIVE_TIMEOUT 100
#define ENABLE_COOLDOWN   100



enum class State {
    BROADCAST,
    DISABLED,
    ENABLED
};



State state = State::BROADCAST;
unsigned long last_time = 0;


Message message;
uint8_t master_addr[ADDRSIZE];


void beep() {}


void send_broadcast_message() {
    Message mesg {BROADCAST};
    esp_now_send(BROADCAST_ADDR, (uint8_t *) &mesg, MESGSIZE);
}


void recv_callback (uint8_t *addr, uint8_t *data, uint8_t len) {
    memcpy(&message, data, len);

    if (message != BROADCAST)
        memcpy(master_addr, addr, ADDRSIZE);
}


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);
}


void handle_broadcast_state() {
    if (millis() - last_time > BROADCAST_DELAY) {
        send_broadcast_message();
        last_time = millis();
    }

    switch (message) {
        case KEEPALIVE_0:
            state = State::DISABLED;
            last_time = millis();
            break;

        case BEEP:
            beep();
            break;
    }
}


void handle_disabled_state() {
    if (millis() - last_time > KEEPALIVE_TIMEOUT) {
        state = State::BROADCAST;
        last_time = millis();
        return;
    }

    switch (message) {
        case KEEPALIVE_0:
            last_time = millis();
            break;

        case KEEPALIVE_1:
            if (millis() - last_time > ENABLE_COOLDOWN) {
                state = State::ENABLED;
                last_time = millis();
            }

            break;

        case BEEP:
            beep();
            break;
    }
}


void handle_enabled_state() {
    if (millis() - last_time > KEEPALIVE_TIMEOUT) {
         state = State::BROADCAST;
         last_time = millis();
         return;
    }

    switch (message) {
        case KEEPALIVE_0:
            state = State::DISABLED;
            last_time = millis();
            break;
            
        case KEEPALIVE_1:
            last_time = millis();
            break;

        case BEEP:
            beep();
            break;
    }
}


void loop() {
    switch (state) {
        case State::BROADCAST:
            handle_broadcast_state();
            break;

        case State::DISABLED:
            handle_disabled_state();
            break;

        case State::ENABLED:
            handle_enabled_state();
            break;
    }

    message = NONE;
}
