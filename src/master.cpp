#include "message.hpp"
#include "utils.hpp"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <espnow.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int n_pairs = 0;
uint8_t connected_address[6];


enum class State: char {
    WAITING_CONN,
    DISABLED_SLAVE,
    ENABLED_SLAVE
};


enum Timings: unsigned long {
    KEEPALIVE_DELAY = 1000,
    PAIRING_DELAY   = 500
};


Timer timer;
State state;
Message last_message;

bool got_new_message = false;
uint8_t last_mac_addr[6];



void display_macs() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    for (int i = 0; i < n_pairs; ++i) {
        u8* ptr = esp_now_fetch_peer(i == 0);

        for (int j = 0; j < 6; ++j) {
            u8 ch = *(ptr + j);
            display.print(ch, HEX);

            if (j < 5)
                display.print(':');
        }

        display.println();
        display.display();
    }
}


void send_disbled_message (uint8_t *mac) {
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0);
        ++n_pairs;
    }

    Message message {Message::FROM_MASTER, Message::DISABLE};
    esp_now_send(mac, (uint8_t *) &message, sizeof(message));
}


void send_keepalive_message (uint8_t *mac) {
    Message message {Message::FROM_MASTER, Message::KEEPALIVE};
    esp_now_send(mac, (uint8_t *) &message, sizeof(message));
}


void recv_callback(uint8_t *mac, uint8_t *data, uint8_t len) {
    memcpy(&last_message, data, MSG_SIZE);

    if (last_message.sender != Message::FROM_SLAVE)
        return;

    memcpy(last_mac_addr, mac, sizeof(last_mac_addr));
    got_new_message = true;
}



void init_display() {
    Wire.begin(4, 5);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(500);
    display.clearDisplay();
}


void init_wifi() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);
}


void setup() {
    init_display();
    init_wifi();
}




/* state handlers */



void handle_waiting_state() {
    if (!got_new_message)
        return;

    switch (last_message.type) {
        case Message::BROADCAST:
            state = State::DISABLED_SLAVE;
            send_disbled_message(last_mac_addr);
            timer.update();
            break;

        default:
            // ERROR I GUESS ?
            break;
    }
}


void handle_disabled_state();
void handle_enabled_state();


void loop() {
    switch (state) {
        case State::WAITING_CONN:
            handle_waiting_state();
            break;

        case State::DISABLED_SLAVE:
            handle_disabled_state();
            break;

        case State::ENABLED_SLAVE:
            handle_enabled_state();
            break;
    }

    got_new_message = false;
}
