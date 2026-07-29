#include <Arduino.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>

#define ESPNOW_WIFI_CHANNEL 6

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

// Motor Pins
int mLeftForward = 15;
int mLeftBackward = 4;
int mRightForward = 21;
int mRightBackward = 22;
const int armUp = 12;
const int armDown = 13;
const int pwmPinLeft = 5;
const int pwmPinRight = 23;
const int mq135 = 36;

volatile bool newMessageAvailable = false;
char incomingMessage[64];

void _onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  int msgLen = min((int)sizeof(incomingMessage) - 1, len);
  memcpy(incomingMessage, data, msgLen);
  incomingMessage[msgLen] = 0;
  newMessageAvailable = true;
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

void processCommand(const char *message) {Serial.printf("Received payload: %s\n", message);
  if (strncmp(message, "M:", 2) != 0) return;

  char driveDir[16] = { 0 };
  char armDir[16] = { 0 };
  int pwm1 = 0;
  int pwm2 = 0;

  int parsed = sscanf(message + 2, "%15[^,],%d,%d,%15s", driveDir, &pwm1, &pwm2, armDir);
  if (parsed < 4) return;

  if (strcmp(driveDir, "FWD") == 0) {
    digitalWrite(mLeftForward, HIGH);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, HIGH);
    digitalWrite(mRightBackward, LOW);
  } else if (strcmp(driveDir, "BWD") == 0) {
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, HIGH);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, HIGH);
  } else if (strcmp(driveDir, "LEFT") == 0) {
    digitalWrite(mLeftForward, HIGH);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, HIGH);
  } else if (strcmp(driveDir, "RIGHT") == 0) {
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, HIGH);
    digitalWrite(mRightForward, HIGH);
    digitalWrite(mRightBackward, LOW);
  } else {
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, LOW);
    pwm1 = 0;
    pwm2 = 0;
  }

  analogWrite(pwmPinLeft, pwm1);
  analogWrite(pwmPinRight, pwm2);

  if (strcmp(armDir, "BOTH_UP") == 0) {
    digitalWrite(armUp, HIGH);
    digitalWrite(armDown, LOW);
  } else if (strcmp(armDir, "BOTH_DOWN") == 0) {
    digitalWrite(armUp, LOW);
    digitalWrite(armDown, HIGH);
  } else {
    digitalWrite(armUp, LOW);
    digitalWrite(armDown, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  if (!espNowBegin()) {
    delay(3000);
    ESP.restart();
  }

  pinMode(mLeftBackward, OUTPUT);
  pinMode(mLeftForward, OUTPUT);
  pinMode(mRightBackward, OUTPUT);
  pinMode(mRightForward, OUTPUT);
  pinMode(pwmPinRight, OUTPUT);
  pinMode(pwmPinLeft, OUTPUT);

  pinMode(armUp, OUTPUT);
  pinMode(armDown, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);

  digitalWrite(armUp, LOW);
  digitalWrite(armDown, LOW);
}

unsigned long lastGasSendTime = 0;

void loop() {
  // Handle incoming messages cleanly in the main loop
  if (newMessageAvailable) {
    newMessageAvailable = false;
    processCommand(incomingMessage);
  }

  // Non-blocking sensor update every 200ms
  if (millis() - lastGasSendTime >= 200) {
    lastGasSendTime = millis();
    int sensorValue = analogRead(mq135);
    float voltage = sensorValue * (3.3 / 4095.0);

    if (voltage <= 0.01) {
      espNowBroadcast("G:0.0");
    } else {
      espNowBroadcast("G:" + String(voltage, 3));
    }
  }
}
