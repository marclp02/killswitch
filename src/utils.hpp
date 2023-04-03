#pragma once


#include <Arduino.h>



uint8_t broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



struct Timer {
    unsigned long last_time = 0;


    bool time_passed(unsigned long dt) {
        unsigned long elapsed = millis() - last_time;
        return (elapsed > dt);
    }


    void update() {
        last_time = millis();
    }
};


