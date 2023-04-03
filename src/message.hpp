#include <Arduino.h>


enum class SMType: uint8_t {
    PAIR_ME_PLZ,
};


enum class MMType: uint8_t {
    I_PAIRED_YOU,
    KEEPALIVE,
};


struct SlaveMsg {
    SMType type;
};


struct MasterMsg {
    MMType type;
};
