#include "utils.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define undefined_slave() eqaddr(slave_addr, BROADCAST_ADDR)


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     1


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int n_pairs = 0;


Packet last_packet;
bool got_new_packet = false;
unsigned long last_time = 0;

uint8_t last_addr[ADDRSIZE];
uint8_t slave_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


/*
void display_addrs() {
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
*/



void display_addrs() {
    for (int i = 0; i < n_pairs; ++i) {
        u8 *ptr = esp_now_fetch_peer(i == 0);

        for (int j = 0; j < 6; ++j) {
            u8 ch = *(ptr + j);
            Serial.print(ch, HEX);

            if (j < 5)
                Serial.print(':');
        }

        Serial.println();
    }
}



void beep() {
    
}




void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    memcpy(&last_packet, in_data, PACKSIZE);
    memcpy(last_addr, addr, ADDRSIZE);
    
    if (undefined_slave() || eqaddr(addr, slave_addr))
        got_new_packet = true;
}




void setup() {
    Wire.begin(4, 5);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(500);
    display.clearDisplay();


    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);

    Serial.begin(9600);
    while (!Serial) {}
}




// TODO
// display reset here !!!
void handle_undef_slave() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;

    if (elapsed > DISPLAY_DELAY) {
        display_addrs();
        last_time = curr_time;
    }

    // maybe choose slave

    if (!got_new_packet)
        return;


    switch (last_packet.type) {
        case BROADCAST_PACK:
            esp_now_add_peer(last_addr, ESP_NOW_ROLE_COMBO, 0, nullptr, 0);
            ++n_pairs;
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
    if (undefined_slave()) {
        handle_undef_slave();
    } else if (got_new_packet && last_packet.type == RECONN_PACK) {
        handle_reconn();
    }


    // button interupts ???

    got_new_packet = false;
}
