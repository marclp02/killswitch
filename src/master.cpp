#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct Message {
    int number;
};

Message data_in;
Message data_out;

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("SENT");
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    data_in = *(Message *)incomingData;
    Serial.print("RECEIVED: ");
    Serial.println(data_in.number);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    if ((millis() - lastTime) > timerDelay) {
        data_out.number = random(1,20);
        esp_now_send(broadcastAddress, (uint8_t *) &data_out, sizeof(data_out));
        lastTime = millis();
    }
}
