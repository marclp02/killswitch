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
Message last_message;

bool got_new_message = false;
uint8_t master_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // CHANGE IT!!!



void beep();




void recv_callback(uint8_t *mac, uint8_t *data, uint8_t len) {
    memcpy(&last_message, data, MSG_SIZE);

    if (last_message.sender == Message::FROM_SLAVE)
        return;

    memcpy(master_addr, mac, sizeof(master_addr));
    got_new_message = true;
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
    if (timer.time_passed(BROADCAST_DELAY)) {
        Message message {Message::FROM_SLAVE, Message::BROADCAST};
        esp_now_send(broadcast_address, (uint8_t *) &message, MSG_SIZE);
        timer.update();
    }

    if (!got_new_message)
        return;

    switch (last_message.type) {
        case Message::DISABLE:
            state = State::DISABLED;
            timer.update();
            break;

        case Message::BEEP:
            beep();
            break;

        default:
            // ERROR I GUESS ?
            return;
    }
}



void handle_disabled_state() {
    if (timer.time_passed(DISABLED_TIMEOUT)) {
        state = State::BROADCAST;
        timer.update();
        return;
    }

    if (!got_new_message)
        return;

    switch (last_message.type) {
        case Message::KEEPALIVE:
            if (timer.time_passed(ENABLED_COOLDOWN)) {
                timer.update();
                state = State::ENABLED;
            }

            break;

        case Message::BEEP:
            beep();
            break;

        default:
            // ERROR I GUESS ?
            break;
    }
}



void handle_enabled_state() {
    if (timer.time_passed(ENABLED_TIMEOUT)) {
        state = State::DISABLED;
        timer.update();
        return;
    }


    if (timer.time_passed(KEEPALIVE_DELAY)) {
        Message message {Message::FROM_SLAVE, Message::DATA};
        esp_now_send(master_addr, (uint8_t *) &message, MSG_SIZE);
        timer.update();
    }

    if (!got_new_message)
        return;
    
    switch (last_message.type) {
        case Message::DISABLE:
            state = State::DISABLED;
            break;

        case Message::BEEP:
            beep();
            break;

        default:
            // ERROR I GUESS ?
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

    got_new_message = false;
}
