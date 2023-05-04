#include "utils.hpp"

#include <ESP8266WiFi.h>
#include <espnow.h>

#include <EasyBuzzer.h>


#define PIEZO_PIN 3
#define ENABLE_PIN 2
#define LED_PIN 1

#define BROADCAST_DELAY   200
#define KEEPALIVE_TIMEOUT 500
#define ENABLE_COOLDOWN   1000

enum class State {
    BROADCAST,
    DISABLED,
    ENABLED
};

State state;

unsigned long last_keepalive = 0;
unsigned long last_broadcast = 0;

void blink() {
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
}

void send_broadcast_message() {
    Message message {BROADCAST};
    esp_now_send(BROADCAST_ADDR, (uint8_t *) &message, MESGSIZE);
}

void to_broadcast() {
    state = State::BROADCAST;
    EasyBuzzer.beep(500, 1);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(ENABLE_PIN, HIGH);
}

void to_disable() {
    state = State::DISABLED;
    EasyBuzzer.beep(500, 2);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(ENABLE_PIN, HIGH);
}

void to_enable() {
    state = State::ENABLED;
    EasyBuzzer.beep(500, 3);
    //delay(ENABLE_COOLDOWN);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(ENABLE_PIN, LOW);
}

void recv_callback (uint8_t *addr, uint8_t *data, uint8_t len) {
    if (data[0] == BEEP) {
        //asyBuzzer.beep(330, 5);
    } else if (data[0] == KEEPALIVE_0) {
        last_keepalive = millis();
        if (state != State::DISABLED) {
            to_disable();
        }
    } else if (data[0] == KEEPALIVE_1) {
        last_keepalive = millis();
        if (state != State::ENABLED) {
            to_enable();
        }
    }
}


void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    EasyBuzzer.setPin(PIEZO_PIN);

    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);

    to_broadcast();
}

void loop() {
    EasyBuzzer.update();

    if ((state == State::ENABLED || state == State::DISABLED) && (millis() - last_keepalive) > KEEPALIVE_TIMEOUT) {
        to_broadcast();
    }

    if (state == State::BROADCAST && (millis() - last_broadcast) > BROADCAST_DELAY) {
        send_broadcast_message();
        blink();
        last_broadcast = millis();
    }
}
