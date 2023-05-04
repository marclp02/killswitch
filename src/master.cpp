#include "utils.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <avdweb_Switch.h>

#define BUTTON_OK 3
#define BUTTON_KILL 1

#define MAX_PEERS 5
#define SCREEN_WIDTH_CHARS 21

#define BUTTON_INTERVAL 100
#define KEEPALIVE_INTERVAL 100
#define SCREEN_INTERVAL 200

#define MAXHASH         349
#define PEERTIMEOUT     1000


enum class State {
    SEARCH,
    SEND
};

Adafruit_SSD1306 display(128, 64, &Wire, -1);

Switch Ok(BUTTON_OK);

State state;
bool keepalive = false;

int animation_counter = 0;

uint8_t slave_addr[] = {0x40, 0xF5, 0x20, 0x25, 0x32, 0x64};
int peer_chosen = 0;
int peer_count = 0;

bool send_success = true;

unsigned long last_keepalive_update = 0;
unsigned long last_button_update = 0;
unsigned long last_screen_update = 0;
unsigned long peer_last_time[MAXHASH];

unsigned int hash_addr(const uint8_t *addr)
{
    unsigned int h = 0;
    unsigned b = 97;

    for (int i = 0; i < 6; ++i)
        h = (h * b + addr[i]) % MAXHASH;

    return h;
}

void delete_inactive_peers()
{
    for (int i = 0; i < peer_count; ++i) {
        uint8_t *addr = esp_now_fetch_peer(i == 0);
        if ((millis() - peer_last_time[hash_addr(addr)]) > PEERTIMEOUT) {
            esp_now_del_peer(addr);
            --peer_count;
            peer_chosen = min(peer_chosen, max(0, peer_count - 1));
        }
    }
}

void update_animation()
{
    animation_counter = (animation_counter + 1) % SCREEN_WIDTH_CHARS;
}

void show_animation()
{
    display.setTextSize(1);
    display.setCursor(0, 56);
    for (int i = 0; i < animation_counter; ++i) {
        display.print("*");
    }
}

void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len)
{
    if (state == State::SEARCH && in_data[0] == BROADCAST) {
        update_animation();
        peer_last_time[hash_addr(addr)] = millis();
        if (!esp_now_is_peer_exist(addr) && peer_count < MAX_PEERS) {
            esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
            peer_count++;
        }
    }
}

void sent_callback(uint8_t *addr, uint8_t status)
{
    update_animation();
    send_success = (status == 0);
}

void next_peer()
{
    if (peer_count > 0) {
        peer_chosen++;
        if (peer_chosen == peer_count){
            peer_chosen = 0;
        }
    }
}

void choose_peer()
{
    uint8_t *addr = NULL;

    for (int i = 0; i <= peer_chosen; ++i)
        addr = esp_now_fetch_peer(i == 0);

    if (addr != NULL) {
        memcpy(slave_addr, addr, 6);
    }
}

void send_keepalive() {
    Message message = KEEPALIVE_0;
    if (keepalive) {
        message = Message::KEEPALIVE_1;
    }
    esp_now_send(slave_addr, (uint8_t *) &message, 1);
}

// State Changers
void to_search()
{
    state = State::SEARCH;
    keepalive = false;
}

void to_send()
{
    state = State::SEND;
    keepalive = false;
    choose_peer();
}

// Display artits
void update_display_search() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SELECT SLAVE:");
    display.println("---------------------");

    for (int i = 0; i < peer_count; ++i) {
        uint8_t *addr = esp_now_fetch_peer(i == 0);
        
        if (i == peer_chosen)
            display.print("[*] ");
        else
            display.print("[ ] ");

        for (int j = 0; j < 6; ++j) {
            display.print(addr[j], HEX);

            if (j < 5)
                display.print(':');
        }

        display.println();
    }

    display.println();
}

void update_display_send() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("KILLSWITCH v0.1b");
    for (int j = 0; j < 6; ++j) {
        display.print(slave_addr[j], HEX);
        if (j < 5)
            display.print(':');
    }
    display.println();
    display.println("---------------------");
    display.println("|                   |");
    display.println("|                   |");
    display.println("|                   |");
    display.println("---------------------");

    display.setTextSize(2);
    if (send_success && keepalive) {
        display.setCursor(48, 28);
        display.print("ON");
    }
    else if (send_success && !keepalive) {
        display.setCursor(44, 30);
        display.print("OFF");
    }
    else if (!send_success) {
        display.setCursor(36, 30);
        display.print("RECON");
    }
}

void setup() {
    // Display
    Wire.begin(2, 0);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("     KILLSWITCH     ");
    display.println("       v0.1b        ");
    display.display();

    // Pins
    pinMode(BUTTON_KILL, INPUT_PULLUP);

    // ESP-NOW
    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    esp_now_register_send_cb(sent_callback);
    esp_now_register_recv_cb(recv_callback);

    to_search();

}

void loop() {
    // Handle buttons

    if ((millis() - last_button_update) > BUTTON_INTERVAL) {
        Ok.poll();
        switch (state) {
            case State::SEARCH:
                if (Ok.longPress()) {
                    if (peer_count > 0) {
                        to_send();
                    }
                }
                else if (Ok.singleClick()) {
                    next_peer();
                }
                break;
            case State::SEND:
                if (Ok.singleClick()) {
                    to_search();
                }
                break;
        }
    }

    if (state == State::SEND && (millis() - last_keepalive_update) > KEEPALIVE_INTERVAL) {
        keepalive = (digitalRead(BUTTON_KILL) == 1);
        send_keepalive();
        last_keepalive_update = millis();
    }

    if ((millis() - last_screen_update) > SCREEN_INTERVAL) {
        switch (state) {
            case State::SEARCH:
                //delete_inactive_peers();
                update_display_search();
                break;
            case State::SEND:
                update_display_send();
                break;
        }
        show_animation();
        display.display();
        last_screen_update = millis();
    }

}
