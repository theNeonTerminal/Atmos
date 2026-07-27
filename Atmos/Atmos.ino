#include <Arduino.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>

#define ESPNOW_WIFI_CHANNEL 6  // must match on every unit


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

// ==== YOUR CODE ====
int mLeftForward = 15;
int mLeftBackward = 4;
int mRightForward = 21;
int mRightBackward = 22;
const int armUp = 12;
const int armDown = 13;

int received_pwm1 = 0;
int received_pwm2 = 0;

const int pwmPinLeft = 5;
const int pwmPinRight = 23;

void onEspNowMessage(const char *message) {
  Serial.printf("Received payload: %s\n", message);

  char driveDir[16] = { 0 };
  char armDir[16] = { 0 };
  int pwm1 = 0;
  int pwm2 = 0;

  // Parse payload string formatted as "DRIVE_DIR,PWM1,PWM2,ARM_DIR"
  int parsed = sscanf(message, "%[^,],%d,%d,%[^,]", driveDir, &pwm1, &pwm2, armDir);

  if (parsed < 4) {
    Serial.println("Error: Failed to fully parse packet.");
    return;
  }

  received_pwm1 = pwm1;
  received_pwm2 = pwm2;

  // --- 1. DRIVE BEHAVIORS ---
  if (strcmp(driveDir, "FWD") == 0) {
    digitalWrite(mLeftForward, HIGH);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, HIGH);
    digitalWrite(mRightBackward, LOW);

    analogWrite(pwmPinLeft, received_pwm1);
    analogWrite(pwmPinRight, received_pwm2);
  } else if (strcmp(driveDir, "BWD") == 0) {
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, HIGH);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, HIGH);

    analogWrite(pwmPinLeft, received_pwm1);
    analogWrite(pwmPinRight, received_pwm2);
  } else if (strcmp(driveDir, "LEFT") == 0) {
    digitalWrite(mLeftForward, HIGH);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, HIGH);

    analogWrite(pwmPinLeft, received_pwm1);
    analogWrite(pwmPinRight, received_pwm2);
  } else if (strcmp(driveDir, "RIGHT") == 0) {
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, HIGH);
    digitalWrite(mRightForward, HIGH);
    digitalWrite(mRightBackward, LOW);

    analogWrite(pwmPinLeft, received_pwm1);
    analogWrite(pwmPinRight, received_pwm2);
  } else {
    // STOP state
    digitalWrite(mLeftForward, LOW);
    digitalWrite(mLeftBackward, LOW);
    digitalWrite(mRightForward, LOW);
    digitalWrite(mRightBackward, LOW);

    analogWrite(pwmPinLeft, 0);
    analogWrite(pwmPinRight, 0);
  }

  // --- 2. ARM BEHAVIORS ---
  if (strcmp(armDir, "BOTH_UP") == 0) {
    digitalWrite(armUp, HIGH);
    digitalWrite(armDown, LOW);
  } else if (strcmp(armDir, "BOTH_DOWN") == 0) {
    digitalWrite(armUp, LOW);
    digitalWrite(armDown, HIGH);
  } else {
    // Default STAY state (and automatically filters out R_UP_L_DN / L_UP_R_DN)
    digitalWrite(armUp, LOW);
    digitalWrite(armDown, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  if (!espNowBegin()) {
    Serial.println("ESP-NOW init failed, rebooting...");
    delay(3000);
    ESP.restart();
  }

  // Drive pins setup
  pinMode(mLeftBackward, OUTPUT);
  pinMode(mLeftForward, OUTPUT);
  pinMode(mRightBackward, OUTPUT);
  pinMode(mRightForward, OUTPUT);
  pinMode(pwmPinRight, OUTPUT);
  pinMode(pwmPinLeft, OUTPUT);

  // Arm pins setup
  pinMode(armUp, OUTPUT);
  pinMode(armDown, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);

  // Initialize all outputs to safe LOW state
  digitalWrite(armUp, LOW);
  digitalWrite(armDown, LOW);

  Serial.println("Setup complete.");
}

// Ammonia (NH₃) – fertilizer, refrigeration leaks Carbon dioxide (CO₂) – approximate response only; not accurate enough for measuring CO₂ concentration
// Nitrogen oxides (NOₓ) – vehicle exhaust, combustion
// Benzene and other VOCs – fuels, solvents, paints
// Alcohol vapors – ethanol, isopropanol
// Smoke and combustion byproducts – fires, wildfires

void loop() {
  // Intentionally empty for the receiver
}
