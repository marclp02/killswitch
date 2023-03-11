#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t broadcastAddress[] = {0xE8, 0xDB, 0x84, 0x99, 0xFE, 0x1D};

typedef struct struct_message {
    String message;
} struct_message;

struct_message data_in;
struct_message data_out;

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
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.println(data_in.message);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, nullptr, 0);

    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);


}

void loop() {
    if (Serial.available() > 0) {
        data_out.message = Serial.readString();
        esp_now_send(broadcastAddress, (uint8_t *) &data_out, sizeof(data_out));
    }
    yield();
}