#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>


#include "utils.hpp"
#include "message.hpp"




enum class State: char {
   BROADCAST,
   DISABLED,
   ENABLED
};




enum Timings: unsigned long {
    BROADCAST_DELAY = 500,
    KEEPALIVE_DELAY = 1000,

    DISABLED_TIMEOUT = 5000,

    ENABLED_TIMEOUT  = 100,
    ENABLED_COOLDOWN = 1000
};





State state;
Timer timer;
MasterMessage last_message;



void recv_callback(uint8_t *mac, uint8_t *data, uint8_t len) {
    memcpy(&last_message, data, MSG_SIZE);

    switch (last_message) {
        case MasterMessage::DISABLE:
            state = State::DISABLED;
            break;

        case MasterMessage::KEEPALIVE:
            state = State::ENABLED;
            break;

        case MasterMessage::BEEP:
            // TODO
            break;
    }

    timer.update();
}



void setup() {
    /*  init WiFi   */
    WiFi.mode(WIFI_STA);
    esp_now_init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);

    /*  init slave state    */
    state = State::BROADCAST;
}



/* state handlers */



void handle_broadcast_state() {
    SlaveMessage message = SlaveMessage::BROADCAST;
    esp_now_send(broadcast_address, (uint8_t *) &message, MSG_SIZE);
}



void handle_disabled_state() {
    // TODO
}



void handle_enabled_state() {
    if (timer.time_passed())
    // TODO
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
}
