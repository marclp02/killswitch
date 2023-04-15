#include "utils.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_UP 14
#define BUTTON_MIDDLE 12
#define BUTTON_DOWN 13

#define KEEPALIVE_INTERVAL 200
#define DEBOUNCE_DELAY 100


enum class State {
    SEARCH,
    DISABLED,
    ENABLED,
    RECONNECT
};

enum class Button {
    NONE,
    UP,
    OK,
    DOWN,
    KILL_DOWN,
    KILL_UP
};



Adafruit_SSD1306 display(128, 64, &Wire, -1);


State state = State::SEARCH;
uint8_t slave_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int n_peers = 0;
int selected_peer = 0;

unsigned long last_time = 0;

Button button = Button::NONE;
unsigned long last_button_press = 0;

void print_slave_list(int index) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SELECT SLAVE:");
    display.setTextSize(1);
    display.println("--------------------------");
    for (int i = 0; i < n_peers; ++i) {
        u8* ptr = esp_now_fetch_peer(i == 0);
        if (i == index) {
            display.print("[*]");
        } else {
            display.print("[ ]");
        }

        for (int j = 0; j < 6; ++j) {
            u8 ch = *(ptr + j);
            display.print(ch, HEX);

            if (j < 5)
                display.print(':');
        }
    }
    display.println();

}

void print_addrs() {
    for (int i = 0; i < n_peers; ++i) {
        u8 *ptr = esp_now_fetch_peer(i == 0);

        for (int j = 0; j < 6; ++j) {
            u8 ch = *(ptr + j);
            Serial.print(ch, HEX);

            if (j < 5)
                Serial.print(':');
        }

        Serial.println();
    }

    Serial.println();
}

void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    if (in_data[0] == BROADCAST && !esp_now_is_peer_exist(addr)) {
        esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
        ++n_peers;
    }
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
        button = Button::KILL_DOWN;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_kill_up() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::KILL_UP;
        last_button_press = millis();
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
    pinMode(BUTTON_UP, INPUT);
    pinMode(BUTTON_MIDDLE, INPUT);
    pinMode(BUTTON_DOWN, INPUT);

}




// TODO
void handle_undef_slave() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;

    if (elapsed > DISPLAY_DELAY) {
        display_addrs();
        print_addrs();
        clear_esp_list();
        last_time = curr_time;
    }
    
    // maybe choose slave

    if (!got_new_packet)
        return;


    switch (last_packet.type) {
        case BROADCAST_PACK:
            if (!esp_now_is_peer_exist(last_addr)) {
                esp_now_add_peer(last_addr, ESP_NOW_ROLE_COMBO, 0, nullptr, 0);
                ++n_pairs;
            }

            break;

        case BEEP_PACK:
            beep();
            break;
    }
}



void handle_reconn() {
    Packet packet {RECONN_PACK};
    esp_now_send(slave_addr, (uint8_t *) &packet, PACKSIZE);
}



void loop() {
    // Search



    if (undefined_slave()) {
        handle_undef_slave();
    } else if (got_new_packet && last_packet.type == RECONN_PACK) {
        handle_reconn();
    }


    // button interupts ???

    got_new_packet = false;
}
