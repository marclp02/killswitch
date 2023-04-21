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
    SEND
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
int animation_counter = 0;


bool keepalive = false;

uint8_t slave_addr[] = {0x40, 0xF5, 0x20, 0x25, 0x32, 0x64};
int peer_chosen = 0;
int peer_count = 0;

bool send_succes = true;

unsigned long last_time = 0;

Button button = Button::NONE;
bool kill_is_pressed = false;
unsigned long last_button_press = 0;

void serial_print_addrs();

void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    if (in_data[0] == BROADCAST && !esp_now_is_peer_exist(addr)) {
        serial_print_addrs();
        esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
        peer_count++;
        update = true;
    }
}

void sent_callback(uint8_t *addr, uint8_t status) {
    send_succes = (status == 0);
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
        case Button::UP:
            update = true;
            peer_chosen = max(peer_chosen - 1, 0);
            break;
        case Button::DOWN:
            update = true;
            peer_chosen = min(peer_chosen + 1, peer_count - 1);
            break;
        case Button::OK:
            update = true;
            if (peer_count > 0 && choose_slave()) {
                update = true;
                keepalive = false;
                state = State::SEND;
            }

            break;
    }
}

void send_handle_buttons() {
    switch (button) {
        case Button::NONE:
        case Button::UP:
        case Button::DOWN:
            break;
        case Button::OK:
            keepalive = false;
            state = State::SEARCH;
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
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_KILL, INPUT_PULLUP);

    // Interrupts
    attachInterrupt(BUTTON_UP, isr_up, FALLING);
    attachInterrupt(BUTTON_OK, isr_ok, FALLING);
    attachInterrupt(BUTTON_DOWN, isr_down, FALLING);

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
    display.printf("SLAVE:");
    for (unsigned char & i : slave_addr) {
        display.print(i, HEX);
    }
    display.println();
    display.println("---------------------");
    display.println("|                   |");
    display.println("|                   |");
    display.println("|                   |");
    display.println("|                   |");
    display.println("---------------------");

    display.setTextSize(2);
    if (send_succes && keepalive) {
        display.setCursor(64, 32);
        display.print("ON");
    }
    else if (send_succes && !keepalive) {
        display.setCursor(64, 32);
        display.print("OFF");
    }
    else if (!send_succes) {
        display.setCursor(64, 32);
        display.print("RECON");
    }

}



void loop() {
    unsigned long elapsed = millis() - last_time;
    switch (state) {
        case State::SEARCH:
            search_handle_buttons();
            break;
        case State::SEND:
            send_handle_buttons();
            if (!send_succes) {
                keepalive = false;
            }
            if (elapsed > KEEPALIVE_INTERVAL) {
                send_keepalive(keepalive);
                last_time = millis();
                update = true;
            }
            break;
    }

    if (update) {
        animation_counter = (animation_counter + 1) % 21;
        switch (state) {
            case State::SEARCH:
                update_display_search();
                break;
            case State::SEND:
                update_display_send();
                break;
        }
        /*
        display.setTextSize(1);
        display.setCursor(0, 10);
        for (int i = 0; i < animation_counter; ++i) {
            display.print("*");
        }
         */
        display.display();

        update = false;

        // TODO: update screen

        // Search: diplays list of found peers

        // Send:
        //  - ON: Displays peer's mac, ON and a nice animation
        //  - OFF Displays peer's mac, OFF and a nice animation
        //  - RECONNECT: Displays peer's mac, "CONNECTING" and progress bar
    }
}
