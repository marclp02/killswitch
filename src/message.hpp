#include<Arduino.h>

enum MType : uint8_t {
    SEND_PAIRING,
    RECEIVE_PAIRING,
    BEEP,
    KEEP_ALIVE,
};

struct Message {
    MType mType;
    int value;
};

