#include <ESP8266WiFi.h>
#include <espnow.h>

typedef struct struct_message {
      char a[32];
      int b;
      float c;
      String d;
      bool e;
} struct_message;

struct_message data_in
;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    memcpy(&data_in, incomingData, sizeof(data_in));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Char: ");
    Serial.println(data_in.a);
    Serial.print("Int: ");
    Serial.println(data_in.b);
    Serial.print("Float: ");
    Serial.println(data_in.c);
    Serial.print("String: ");
    Serial.println(data_in.d);
    Serial.print("Bool: ");
    Serial.println(data_in.e);
    Serial.println();
}
 
void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != 0) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    delay(500);
    Serial.println("setup...");
    
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}
