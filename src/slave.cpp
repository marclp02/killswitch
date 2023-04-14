#include "utils.h"

#include <ESP8266WiFi.h>
#include <espnow.h>




int state = BROADCAST_STATE;
unsigned long last_time = 0;

Packet last_packet;
bool got_new_packet = false;

uint8_t master_addr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // TODO: change it to real master mac address!




void beep() {

}




void send_broadcast_packet() {
    Packet packet {BROADCAST_PACK};
    esp_now_send(BROADCAST_ADDR, (uint8_t *) &packet, PACKSIZE);
}




void send_reconn_packet() {
    Packet packet {RECONN_PACK};
    esp_now_send(master_addr, (uint8_t *) &packet, PACKSIZE);
}




void recv_callback (uint8_t *addr, uint8_t *in_data, uint8_t len) {
    // be sure to recieve packet from correct device
    if (eqaddr(addr, master_addr)) {
        got_new_packet = true;
        memcpy(&last_packet, in_data, PACKSIZE);
    }
}




void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    WiFi.mode(WIFI_STA);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv_callback);
};




void handle_broadcast_state() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;


    if (elapsed > BROADCAST_DELAY) {
        send_broadcast_packet();
        last_time = curr_time;
    }

    
    if (!got_new_packet)
        return;

    
    switch (last_packet.type) {
        case DISARM_PACK:
            state = DISARMED_STATE;
            last_time = curr_time;
            break;

        case BEEP_PACK:
            beep();
            break;
    }
}




void handle_disarmed_state() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;


    if (elapsed > KEEPALIVE_TIMEOUT) {
        state = RECONN_STATE;
        last_time = curr_time;
        return;
    }

    
    if (!got_new_packet)
        return;


    switch (last_packet.type) {
        case ARM_PACK:
            if (elapsed > ARM_COOLDOWN) {
                state = ARMED_STATE;
                last_time = curr_time;
            }

            break;

        case KEEPALIVE_PACK:
            last_time = curr_time;
            break;

        case BEEP_PACK:
            beep();
            break;
    }
}




void handle_armed_state() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;

    if (elapsed > KEEPALIVE_TIMEOUT) {
        state = RECONN_STATE;
        curr_time = last_time;
        return;
    }


    if (!got_new_packet)
        return;


    switch (last_packet.type) {
        case DISARM_PACK:
            state = DISARMED_STATE;
            break;

        case KEEPALIVE_PACK:
            last_time = curr_time;
            break;
    }
}



void handle_reconn_state() {
    unsigned long curr_time = millis();
    unsigned long elapsed = curr_time - last_time;


    // something wrong with time for sure
    if (elapsed > RECONN_TIMEOUT) {
        state = BROADCAST_STATE;
        last_time = curr_time;
        return;
    }


    if (elapsed > RECONN_DELAY) {
        send_reconn_packet();
        last_time = curr_time;
    }


    if (!got_new_packet)
        return;


    switch (last_packet.type) {
        case RECONN_PACK:
            state = DISARMED_STATE;
            last_time = curr_time;
            break;

        case BEEP_PACK:
            beep();
            break;
    }
}



void loop() {
    switch (state) {
        case BROADCAST_STATE:
            handle_broadcast_state();
            break;

        case DISARMED_STATE:
            handle_disarmed_state();
            break;

        case ARMED_STATE:
            handle_armed_state();
            break;

        case RECONN_STATE:
            handle_reconn_state();
            break;
    }

    got_new_packet = false;
}
