#include <stdint.h>




struct Message {
    constexpr static int DATASIZE = 32;
    
    enum Sender: bool {
        FROM_SLAVE,
        FROM_MASTER
    };

    enum Type {
        /* from master */
        DISABLE,
        KEEPALIVE,
        BEEP,

        /* from slave */
        BROADCAST,
        DATA
    };

    
    Sender sender;
    Type type;
    char data[DATASIZE];
};



#define MSG_SIZE sizeof(Message)
