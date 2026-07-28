#include <Arduino.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <Adafruit_ADS1X15.h>

#define ESPNOW_WIFI_CHANNEL 6  // must match on every unit

Adafruit_ADS1015 ads;

// ---------------------------------------------------------------
// INTERNAL PLUMBING — you never need to touch anything below this
// until the "==== YOUR CODE ====" marker.
// ---------------------------------------------------------------
class _BroadcastPeer : public ESP_NOW_Peer {
public:
  _BroadcastPeer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  ~_BroadcastPeer() {
    remove();
  }

  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      Serial.println("Failed to initialize ESP-NOW or register broadcast peer");
      return false;
    }
    return true;
  }

  bool sendMessage(const String &msg) {
    return send((const uint8_t *)msg.c_str(), msg.length());
  }
};

static _BroadcastPeer _peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

void _onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);

bool espNowBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (!_peer.begin()) return false;

  esp_now_register_recv_cb(_onEspNowRecv);
  return true;
}

bool espNowBroadcast(const String &msg) {
  return _peer.sendMessage(msg);
}

void onEspNowMessage(const char *message);

void _onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min((int)ESP_NOW_MAX_DATA_LEN, len);
  memcpy(buffer, data, msgLen);
  buffer[msgLen] = 0;
  onEspNowMessage(buffer);
}

void onEspNowMessage(const char *message) {
  // Transmitter doesn't strictly need to do anything here, but we'll leave it
  Serial.printf("Received: %s\n", message);
}

// ==========================================
// ==== TRANSMITTER SETUP & LOOP CODE ====
// ==========================================

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 50;  // Send updates every 50ms

// Define limits based on typical ADS1015 GAIN_ONE ranges (approx 0 to 1600+ depending on voltage)
// We will cap the "upper limit" at 1600 for mapping calculations.
const int LOWER_LIMIT = 0;
const int UPPER_LIMIT = 1600;

void setup(void) {
  Serial.begin(115200);
  ads.setGain(GAIN_ONE);

  if (!espNowBegin()) {
    Serial.println("ESP-NOW init failed, rebooting...");
    delay(3000);
    ESP.restart();
  }
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  Serial.println("Transmitter Setup DONE!");
}

void loop(void) {
  // Read analog values
  int16_t adc1 = ads.readADC_SingleEnded(3);  // Left/Right
  int16_t adc3 = ads.readADC_SingleEnded(1);  // Forward/Backward
  int16_t adc0 = ads.readADC_SingleEnded(2);  // Arm Mix (Right up + Left down / Left up + Right down)
  int16_t adc2 = ads.readADC_SingleEnded(0);  // Both Arms Up/Down

  // --- 1. MOVEMENT CALCULATION (ADC0 & ADC1) ---
  // Default values
  String driveDir = "STOP";
  int pwm1 = 0;  // Speed component 1
  int pwm2 = 0;  // Speed component 2

  // Check Forward/Backward (Priority 1)
  if (adc1 < 650) {
    driveDir = "BWD";
    // Scale 650 -> 0 to 0 -> 255 PWM
    pwm1 = map(adc1, 650, LOWER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  } else if (adc1 > 900) {
    driveDir = "FWD";
    // Scale 900 -> 1600 to 0 -> 255 PWM
    pwm1 = map(adc1, 900, UPPER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  }
  // Check Left/Right (Priority 2, overrides forward/backward)
  else if (adc0 > 900) {
    driveDir = "LEFT";
    pwm1 = map(adc0, 900, UPPER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  } else if (adc0 < 650) {
    driveDir = "RIGHT";
    pwm1 = map(adc0, 650, LOWER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  }

  // --- 2. ARM CALCULATION (ADC2 & ADC3) ---
  String armDir = "STAY";

  // Check Both Arms (Priority 1)
  if (adc3 < 650) {
    armDir = "BOTH_DOWN";
  } else if (adc3 > 900) {
    armDir = "BOTH_UP";
  }
  // Check Split Arms (Priority 2)
  else if (adc2 < 650) {
    armDir = "R_UP_L_DN";
  } else if (adc2 > 900) {
    armDir = "L_UP_R_DN";
  }

  // --- 3. BROADCAST DATA ---
  // Send data periodically to avoid spamming the bandwidth
  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();

    // Protocol Format: "DRIVE_DIR,PWM1,PWM2,ARM_DIR"
    // Example: "FWD,180,180,STAY" or "LEFT,255,255,BOTH_UP"
    String payload = driveDir + "," + String(pwm1) + "," + String(pwm2) + "," + armDir;
    espNowBroadcast(payload);
  }
}
