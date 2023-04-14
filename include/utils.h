#ifndef __UTILS_H__
#define __UTILS_H__


#include <Arduino.h>


#define BROADCAST_STATE 0
#define DISARMED_STATE  1
#define ARMED_STATE     2
#define RECONN_STATE    3

// I put numbers randomly, change them in future ;)
#define DISPLAY_DELAY     200
#define BROADCAST_DELAY   200
#define KEEPALIVE_DELAY   1000
#define KEEPALIVE_TIMEOUT 200
#define RECONN_DELAY      100
#define RECONN_TIMEOUT    5000
#define ARM_COOLDOWN      1000


#define BROADCAST_PACK 0
#define KEEPALIVE_PACK 1
#define ARM_PACK       2
#define DISARM_PACK    3
#define BEEP_PACK      4
#define RECONN_PACK    5


#define DATASIZE 64
#define ADDRSIZE 6
#define PACKSIZE sizeof(Packet)
#define METASIZE (PACKSIZE - DATASIZE)


uint8_t BROADCAST_ADDR[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


struct Packet {
    uint8_t type;
    uint8_t data[DATASIZE];
};



bool eqaddr(uint8_t *a, uint8_t *b) {
    for (int i = 0; i < ADDRSIZE; ++i)
        if (a[i] != b[i])
            return false;

    return true;
}


#endif
