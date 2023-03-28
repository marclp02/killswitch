#include "message.hpp"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <espnow.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int n_pairs = 0;
uint8_t connected_address[6];

unsigned long last_time = 0;
unsigned long pairing_delay = 500;
unsigned long keepalive_delay = 200;



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


void blink(uint8_t dt) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(dt);
    digitalWrite(LED_BUILTIN, HIGH);
}


void confirm_pairing(uint8_t *mac) {
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0);
        ++n_pairs;
    }

    MasterMsg pairing {MMType::PAIR};
    esp_now_send(mac, (uint8_t *) &pairing, sizeof(pairing)); // `mac` or `nullptr` ?
    blink(200);
}


void send_keepalive(uint8_t *mac) {
    MasterMsg keepalive {MMType::KEEPALIVE};
    esp_now_send(mac, (uint8_t *) &keepalive, sizeof(keepalive));
}


void on_data_recv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
    // received from slave
    SlaveMsg message;
    memcpy(&message, incomingData, sizeof(message));

    switch (message.type) {
        case SMType::PAIR:
            confirm_pairing(mac);

        default:
            break;
    }
}


void init_display() {
    Wire.begin(2,0);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(500);
    display.clearDisplay();
}


void init_wifi() {
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(on_data_recv);
}


void setup() {
    init_display();
    init_wifi();
}


void loop() {
    unsigned long elapsed = millis() - last_time;

    if (elapsed > keepalive_delay) {
        display_macs();
        send_keepalive(nullptr);

        last_time = millis();
    }
}
