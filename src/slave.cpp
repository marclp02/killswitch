#include "utils.hpp"

#include <ESP8266WiFi.h>
#include <espnow.h>


#define BROADCAST_DELAY   100
#define KEEPALIVE_TIMEOUT 100
#define ENABLE_COOLDOWN   100



enum class State {
    BROADCAST,
    DISABLED,
    ENABLED
};



State state = State::BROADCAST;
unsigned long last_time = 0;


Message message;
bool message_update = false;
uint8_t master_addr[ADDRSIZE];


void beep() {}


void send_broadcast_message() {
    Message mesg {BROADCAST};
    esp_now_send(BROADCAST_ADDR, (uint8_t *) &mesg, MESGSIZE);
}



