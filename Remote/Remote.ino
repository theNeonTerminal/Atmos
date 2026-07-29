#include <Arduino.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define ESPNOW_WIFI_CHANNEL 6

Adafruit_ADS1015 ads;
const int width = 128, height = 64;
volatile float mq135 = 0.0;
volatile bool displayNeedsUpdate = false;

Adafruit_SH1106G display(width, height, &Wire, -1);

class _BroadcastPeer : public ESP_NOW_Peer {
public:
  _BroadcastPeer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  ~_BroadcastPeer() { remove(); }

  bool begin() {
    if (!ESP_NOW.begin() || !add()) return false;
    return true;
  }

  bool sendMessage(const String &msg) {
    return send((const uint8_t *)msg.c_str(), msg.length());
  }
};

static _BroadcastPeer _peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

void _onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min((int)ESP_NOW_MAX_DATA_LEN, len);
  memcpy(buffer, data, msgLen);
  buffer[msgLen] = 0;

  if (strncmp(buffer, "G:", 2) == 0) {
    float val = 0;
    if (sscanf(buffer + 2, "%f", &val) >= 1) {
      mq135 = val;
      displayNeedsUpdate = true; // Flag display update for loop()
    }
  }
}

bool espNowBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  if (!_peer.begin()) return false;
  esp_now_register_recv_cb(_onEspNowRecv);
  return true;
}

bool espNowBroadcast(const String &msg) {
  return _peer.sendMessage(msg);
}

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 50;
const int LOWER_LIMIT = 0;
const int UPPER_LIMIT = 1600;

void setup(void) {
  Serial.begin(115200);
  ads.setGain(GAIN_ONE);

  if (!espNowBegin()) {
    delay(3000);
    ESP.restart();
  }
  if (!ads.begin()) {
    while (1);
  }

  display.begin(0x3C, true);
  display.clearDisplay();
  display.display();
}

void loop(void) {
  // Update OLED in loop rather than inside ESP-NOW interrupt context
  if (displayNeedsUpdate) {
    displayNeedsUpdate = false;
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 20);
    display.printf("Gas: %.2fV", mq135);
    display.display();
  }

  int16_t adc0 = ads.readADC_SingleEnded(3);
  int16_t adc3 = ads.readADC_SingleEnded(1);
  int16_t adc1 = ads.readADC_SingleEnded(2);
  int16_t adc2 = ads.readADC_SingleEnded(0);

  String driveDir = "STOP";
  int pwm1 = 0;
  int pwm2 = 0;

  if (adc1 < 650) {
    driveDir = "BWD";
    pwm1 = map(adc1, 650, LOWER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  } else if (adc1 > 900) {
    driveDir = "FWD";
    pwm1 = map(adc1, 900, UPPER_LIMIT, 0, 255);
    pwm1 = constrain(pwm1, 0, 255);
    pwm2 = pwm1;
  } else if (adc0 > 900) {
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

  String armDir = "STAY";
  if (adc3 < 650) {
    armDir = "BOTH_DOWN";
  } else if (adc3 > 900) {
    armDir = "BOTH_UP";
  } else if (adc2 < 650) {
    armDir = "R_UP_L_DN";
  } else if (adc2 > 900) {
    armDir = "L_UP_R_DN";
  }

  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();
    String payload = "M:" + driveDir + "," + String(pwm1) + "," + String(pwm2) + "," + armDir;
    espNowBroadcast(payload);
  }
}
