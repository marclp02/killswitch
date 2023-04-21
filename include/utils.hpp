#pragma once

#include <Arduino.h>


uint8_t BROADCAST_ADDR[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


#define ADDRSIZE 6
#define MESGSIZE 1


enum Message: uint8_t {
    NONE,
    BROADCAST,
    KEEPALIVE_0,
    KEEPALIVE_1,
    BEEP
};
