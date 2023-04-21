#include "utils.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_OK 12
#define BUTTON_NEXT 14
#define BUTTON_KILL 13

#define MAX_PAIRS 5

#define KEEPALIVE_INTERVAL 200
#define SCREEN_INTERVAL 150
#define DEBOUNCE_DELAY 50

#define MAXHASH         349
#define PEERTIMEOUT     500


enum class State {
    SEARCH,
    SEND
};

enum class Button {
    NONE,
    NEXT,
    OK
};


Adafruit_SSD1306 display(128, 64, &Wire, -1);


State state = State::SEARCH;


int animation_counter = 0;


bool keepalive = false;

uint8_t slave_addr[] = {0x40, 0xF5, 0x20, 0x25, 0x32, 0x64};
int peer_chosen = 0;
int peer_count = 0;

bool send_success = true;

unsigned long last_time = 0;
unsigned long last_button_press = 0;
unsigned long last_screen_update = 0;
unsigned long last_peer_update = 0;

Button button = Button::NONE;
bool kill_is_pressed = false;

unsigned long peer_last_time[MAXHASH];


bool addrcmp(uint8_t *addr0, uint8_t *addr1) {
    for (int i = 0; i < 6; ++i)
        if (addr0[i] != addr1[i])
            return false;

    return true;
}

unsigned int get_addr_id(uint8_t *addr) {
    unsigned int h = 0;
    unsigned b = 97;

    for (int i = 0; i < 6; ++i)
        h = (h * b + addr[i]) % MAXHASH;

    return h;
}

void update_peer_list() {
    for (int i = 0; i < peer_count; ++i) {
        // this may not work ;(
        uint8_t *addr = esp_now_fetch_peer(i == 0);

        unsigned int idx = get_addr_id(addr);

        if (millis() - peer_last_time[idx] > PEERTIMEOUT) {
            esp_now_del_peer(addr);
            --peer_count;
            peer_chosen = min(peer_chosen, max(0, peer_count - 1));
        }
    }
}


void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    animation_counter = (animation_counter + 1) % 21;
    if (in_data[0] == BROADCAST) {
        peer_last_time[get_addr_id(addr)] = millis();
        if (!esp_now_is_peer_exist(addr) && peer_count < 5) {
            esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
            peer_count++;
        }
    }
}

void sent_callback(uint8_t *addr, uint8_t status) {
    animation_counter = (animation_counter + 1) % 21;
    send_success = (status == 0);
}


void IRAM_ATTR isr_next() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::NEXT;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_ok() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        button = Button::OK;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_kill_down() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        kill_is_pressed = true;
    }
}

void IRAM_ATTR isr_kill_up() {
    if (millis() - last_button_press > DEBOUNCE_DELAY) {
        kill_is_pressed = false;
    }
}

bool choose_slave() {
    uint8_t *addr = NULL;

    for (int i = 0; i <= peer_chosen; ++i)
        addr = esp_now_fetch_peer(i == 0);

    if (addr != NULL) {
        memcpy(slave_addr, addr, 6);
        return true;
    }

    return false;
}

void search_handle_buttons() {
    switch (button) {
        case Button::NONE:
            break;
        case Button::NEXT:
            if (peer_count > 0) {
                peer_chosen = (peer_chosen + 1) % peer_count;
            }
            break;
        case Button::OK:
            if (peer_count > 0 && choose_slave()) {
                keepalive = false;
                state = State::SEND;
                esp_now_unregister_recv_cb();
            }

            break;
    }
}

void send_handle_buttons() {
    switch (button) {
        case Button::NONE:
        case Button::NEXT:
            break;
        case Button::OK:
            keepalive = false;
            state = State::SEARCH;
            esp_now_register_recv_cb(recv_callback);
            break;
    }
}

void send_keepalive(bool val) {
    Message message = KEEPALIVE_0;
    if (val) {
        message = Message::KEEPALIVE_1;
    }
    esp_now_send(slave_addr, (uint8_t *) &message, 1);
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
    esp_now_register_send_cb(sent_callback);

    // Seial
    Serial.begin(115200);

    // Buttons
    pinMode(BUTTON_NEXT, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_KILL, INPUT_PULLUP);

    // Interrupts
    attachInterrupt(BUTTON_NEXT, isr_next, FALLING);
    attachInterrupt(BUTTON_OK, isr_ok, FALLING);

    // Falling and Rising for Kill button
    // TODO: function to set keepalive based on kill_is_pressed
    attachInterrupt(BUTTON_KILL, isr_kill_down, FALLING);
    attachInterrupt(BUTTON_KILL, isr_kill_up, RISING);

}

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
    display.println("KILLSWITCH v0.1a");
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


void loop() {
    switch (state) {
        case State::SEARCH:
            search_handle_buttons();
            if ((millis() - last_peer_update) > PEERTIMEOUT) {
                update_peer_list();
            }
            break;
        case State::SEND:
            send_handle_buttons();
            if (!send_success) {
                keepalive = false;
            }
            if ((millis() - last_time) > KEEPALIVE_INTERVAL) {
                send_keepalive(keepalive);
                last_time = millis();
            }
            break;
    }

    button = Button::NONE;

    if ((millis() - last_screen_update) > SCREEN_INTERVAL) {
        switch (state) {
            case State::SEARCH:
                update_display_search();
                break;
            case State::SEND:
                update_display_send();
                break;
        }
        display.setTextSize(1);
        display.setCursor(0, 56);
        for (int i = 0; i < animation_counter; ++i) {
            display.print("*");
        }
        display.display();
        last_screen_update = millis();
    }
}
