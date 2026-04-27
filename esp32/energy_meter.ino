#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// 🔴 IMPORTANT: Replace with your PC IP
const char* serverURL = "http://192.168.1.5:3000/data";

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= SENSOR =================
int sensorPin = 34;

float current;
float power;
float smoothPower = 0;
float energy = 0;

float offset = 0;
float rate = 8.0;

unsigned long lastTime = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  // WiFi connect
  WiFi.begin(ssid, password);
  lcd.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);

  // Offset calibration
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(sensorPin);
    delay(2);
  }
  offset = sum / 500.0;

  lcd.clear();
  lcd.print("Meter Ready");
  delay(1000);
}

// ================= LOOP =================
void loop() {

  long sum = 0;

  // RMS sampling
  for (int i = 0; i < 200; i++) {
    int value = analogRead(sensorPin);
    float diff = value - offset;
    sum += diff * diff;
    delay(1);
  }

  float rms = sqrt(sum / 200.0);
  float voltage = rms * (3.3 / 4095.0);

  current = voltage / 0.185;

  // Noise filtering
  if (current < 0.02) current = 0;
  if (current > 5) current = 0;

  power = 230.0 * current;

  // Smoothing
  smoothPower = (0.7 * smoothPower) + (0.3 * power);

  // 🔥 No-load detection
  if (smoothPower < 10) {
    smoothPower = 0;
    current = 0;
  }

  // Energy calculation
  if (millis() - lastTime >= 1000) {
    lastTime = millis();
    energy += (smoothPower / 1000.0) * (1.0 / 3600.0);
  }

  // 🔥 Real-time prediction
  float predictedCost = 0;
  if (smoothPower > 10) {
    float monthlyUnits = (smoothPower / 1000.0) * 24 * 30;
    predictedCost = monthlyUnits * rate;
  }

  // ================= LCD =================

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P:");
  lcd.print(smoothPower, 1);
  lcd.print("W");

  lcd.setCursor(0, 1);
  lcd.print("I:");
  lcd.print(current, 2);
  lcd.print("A");

  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Energy:");
  lcd.print(energy, 3);

  lcd.setCursor(0, 1);
  lcd.print("Pred:Rs");
  lcd.print(predictedCost, 0);

  delay(1500);

  // ================= SEND DATA =================

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"current\":" + String(current) + ",";
    json += "\"power\":" + String(smoothPower) + ",";
    json += "\"energy\":" + String(energy) + ",";
    json += "\"prediction\":" + String(predictedCost);
    json += "}";

    http.POST(json);
    http.end();
  }

  // Serial debug
  Serial.print("Current: ");
  Serial.print(current);
  Serial.print(" A | Power: ");
  Serial.print(smoothPower);
  Serial.print(" W | Energy: ");
  Serial.print(energy);
  Serial.print(" kWh | Predicted: ");
  Serial.println(predictedCost);
}