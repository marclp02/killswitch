#include <cstdint>

struct struct_pairing {
    uint8_t mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
};

struct struct_keepalive {
    uint8_t keepalive = 1;
};