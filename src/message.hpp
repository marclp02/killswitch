#include <stdint.h>



enum class MasterMessage: char {
    DISABLE,
    KEEPALIVE,
    BEEP
};


enum class SlaveMessage: char {
    BROADCAST,
    HELLOWORLD
};


#define MSG_SIZE sizeof(MasterMessage)
