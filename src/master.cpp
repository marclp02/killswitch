#include "utils.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_UP 14
#define BUTTON_OK 12
#define BUTTON_DOWN 13
#define BUTTON_KILL 00  //TODO: Set kill button gpio pin

#define KEEPALIVE_INTERVAL 200
#define DEBOUNCE_DELAY 100


enum class State {
    SEARCH,
    SEND,
    RECONNECT
};

enum class Button {
    NONE,
    UP,
    OK,
    DOWN
};


Adafruit_SSD1306 display(128, 64, &Wire, -1);


State state = State::SEARCH;


bool update = true;
bool keepalive = false;

uint8_t slave_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool peer_found = false;
int peer_chosen = 0;
int peer_count = 0;


unsigned long last_time = 0;

Button button = Button::NONE;
bool kill_is_pressed = false;
unsigned long last_button_press = 0;

void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    if (in_data[0] == BROADCAST && !esp_now_is_peer_exist(addr)) {
        esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
        peer_count++;
        update = true;
    }
}

void sent_callback(uint8_t *addr, uint8_t status) {
    if (status != )
}


void IRAM_ATTR isr_up() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::UP;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_ok() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::OK;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_down() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::DOWN;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_kill_down() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        kill_is_pressed = true;
        update = true;
    }
}

void IRAM_ATTR isr_kill_up() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        kill_is_pressed = false;
        update = true;
    }
}


void setup() {
    // Display
    Wire.begin(4, 5);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("KILLSWITCH V0.1");
    display.display();

    // ESP-NOW
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);

    // Seial
    Serial.begin(115200);

    // Buttons
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_KILL, INPUT_PULLUP);

    // Interrupts
    attachInterrupt(BUTTON_UP, isr_up, FALLING);
    attachInterrupt(BUTTON_OK, isr_ok, FALLING);
    attachInterrupt(BUTTON_DOWN, isr_down, FALLING);

    // Falling and Rising for Kill button
    attachInterrupt(BUTTON_KILL, isr_kill_down, FALLING);
    attachInterrupt(BUTTON_KILL, isr_kill_up, RISING);

}




void loop() {
    unsigned long elapsed = millis() - last_time;

    if (state == State::SEARCH && (update || button != Button::NONE)) {
        if (button == Button::UP) {
            peer_chosen = max(peer_chosen - 1, 0);
        }
        else if (button == Button::DOWN) {
            peer_chosen = min(peer_chosen + 1, peer_count - 1);
        }
        else if (button == Button::OK && peer_count > 0) {

            for (int i = 1; i < peer_chosen; ++i) {

            }
            memccpy(slave_addr, )
            // Change to paired
            update = true;
            keepalive = false;
            state = State::SEND;
        }
        // Update screen
    }

    if (state == State::SEND) {
        if (elapsed > KEEPALIVE_INTERVAL) {
            // Send KeepAlive 0 or 1
        }
        if (update) {
            // Show instructions on screen
            // Register callback
            // Register Interrupts
        }
    }

    if (state == State::RECONNECT) {
        if (elapsed > KEEPALIVE_INTERVAL) {
            // Send KeepAlive 0
        }
    }

}
