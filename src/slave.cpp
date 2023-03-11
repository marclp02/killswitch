#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
    int number;
} struct_message;

struct_message data_in;
struct_message data_out;

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;  // send readings timer

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0){
        Serial.println("Delivery success");
    }
    else{
        Serial.println("Delivery fail");
    }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    memcpy(&data_in, incomingData, sizeof(data_in));
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

    //esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);
}

void loop() {
    if ((millis() - lastTime) > timerDelay) {
        data_out.number = random(1,20);
        esp_now_send(broadcastAddress, (uint8_t *) &data_out, sizeof(data_out));
        lastTime = millis();
    }
    yield();
}