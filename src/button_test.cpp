#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_UP 14
#define BUTTON_MIDDLE 12
#define BUTTON_DOWN 13

unsigned long last = 0;


Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool change = true;
struct states {
    int UP;
    int MIDDLE;
    int DOWN;
};

states button{1, 1, 1};

unsigned long last_button_press = 0;
unsigned long debounce_delay = 15;

void IRAM_ATTR isr_up() {
    if (millis() - last_button_press > debounce_delay) {
        button.UP = 1 - button.UP;
        change = true;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_reset_debounce() {
    last_button_press = millis();
}

void IRAM_ATTR isr_middle() {
    if (millis() - last_button_press > debounce_delay) {
        button.MIDDLE = 1 - button.MIDDLE;
        change = true;
        last_button_press = millis();
    }
}

void IRAM_ATTR isr_down() {
    if (millis() - last_button_press > debounce_delay) {
        button.DOWN = 1 - button.DOWN;
        change = true;
        last_button_press = millis();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_UP, INPUT);
    pinMode(BUTTON_MIDDLE, INPUT);
    pinMode(BUTTON_DOWN, INPUT);
    attachInterrupt(BUTTON_UP, isr_up, FALLING);
    //attachInterrupt(BUTTON_UP, isr_reset_debounce, RISING);


    attachInterrupt(BUTTON_MIDDLE, isr_middle, FALLING);
    //attachInterrupt(BUTTON_MIDDLE, isr_reset_debounce, RISING);

    attachInterrupt(BUTTON_DOWN, isr_down, FALLING);
    //attachInterrupt(BUTTON_DOWN, isr_reset_debounce, RISING);

    Wire.begin(4, 5);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(500);
    display.clearDisplay();
}

void loop() {
    if (change) {
        Serial.print(button.UP);
        Serial.print(" ");
        Serial.print(button.MIDDLE);
        Serial.print(" ");
        Serial.println(button.DOWN);
        change = false;
    }
}