#include <cstdint>

enum MessageType {PAIRING, KEEPALIVE};


typedef struct struct_pairing {
    uint8_t mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};;
} struct_pairing;

typedef struct struct_keepalive {
    uint8_t msgType = KEEPALIVE;
    uint8_t keepalive = 1;
} struct_keepalive;