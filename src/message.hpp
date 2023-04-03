#include <stdint.h>



enum class MasterMessage: char {
    DISABLE,
    KEEPALIVE,
    BEEP
};


enum class SlaveMessage: char {
    BROADCAST,
};


#define MSG_SIZE sizeof(MasterMessage)
