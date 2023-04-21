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

uint8_t slave_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int peer_chosen = 0;
int peer_count = 0;

bool send_succes = true;

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


void choose_slave() {
    uint8_t *addr;

    for (int i = 0; i < peer_chosen; ++i)
        addr = esp_now_fetch_peer(i == 0);

    memcpy(slave_addr, addr, 6);
}


void search_handle_buttons() {
    switch (button) {
        case Button::NONE:
            break;
        case Button::UP:
            peer_chosen = max(peer_chosen - 1, 0);
            break;
        case Button::DOWN:
            peer_chosen = min(peer_chosen + 1, peer_count);
            break;
        case Button::OK:
            if (peer_count > 0) {
                choose_slave();
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

char hexdigit(uint8_t byte) {
    if (byte >= 10)
        return (byte - 10) + 'a';

    return byte + '0';
}


void pretty_addr(uint8_t *addr, char *buf) {
    int j = 0;

    for (int i = 0; i < 6; ++i) {
        buf[j++] = hexdigit(addr[i] & 0b11110000);
        buf[j++] = hexdigit(addr[i] & 0b00001111);

        if (i != 5)
            buf[j++] = ':';
    }

    buf[j] = 0;
}


void update_display_search() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SELECT SLAVE:");
    display.setTextSize(1);

    display.println("--------------------------");

    for (int i = 0; i < peer_count; ++i) {
        uint8_t *addr = esp_now_fetch_peer(i == 0);
        
        if (i == peer_chosen)
            display.print("[*] ");
        else
            display.print("[ ] ");

        char buf[20];
        pretty_addr(addr, buf);

        display.println(buf);
    }

    display.println();
    display.display();
}

void update_display_send() {
    // ON
    display.clearDisplay();
    display.println("KILLSWITCH v0.1a");
    display.print("SLAVE: ");
    display.printf("%02d:%02d:%02d:%02d:%02d:%02d", slave_addr[0], slave_addr[1], slave_addr[2], slave_addr[3], slave_addr[4], slave_addr[5]);
    display.println();

    display.println("----------------");
    display.println("|              |");
    display.println("|              |");
    display.println("----------------");
    for (int i = 0; i < animation_counter; ++i) {
        display.print("*");
    }

    if (send_succes && keepalive) {
        display.setCursor(6, 4);
        display.setTextSize(2);
        display.print("ON");
    }
    else if (send_succes && !keepalive) {
        display.setCursor(5, 4);
        display.setTextSize(2);
        display.print("ON");
    }
    else if (!send_succes) {
        display.setCursor(5, 3);
        display.setTextSize(2);
        display.print("ON");
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
        animation_counter = (animation_counter + 1) % 16;
        switch (state) {
            case State::SEARCH:
                update_display_search();
                break;
            case State::SEND:
                update_display_send();
                break;
        }

        // TODO: update screen

        // Search: diplays list of found peers

        // Send:
        //  - ON: Displays peer's mac, ON and a nice animation
        //  - OFF Displays peer's mac, OFF and a nice animation
        //  - RECONNECT: Displays peer's mac, "CONNECTING" and progress bar
    }
}
