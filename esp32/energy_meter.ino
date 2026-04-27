#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>   // ✅ FIX for sqrt()

#define SDA_PIN 21
#define SCL_PIN 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

int sensorPin = 34;

float current = 0;
float power = 0;
float smoothPower = 0;
float energy = 0;

float offset = 0;
float rate = 8.0;

unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.begin(16, 2);   // ✅ safer than lcd.init()
  lcd.backlight();

  // Offset calibration
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(sensorPin);
    delay(2);
  }
  offset = sum / 500.0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Energy Meter");
  delay(2000);
}

void loop() {

  long sum = 0;

  for (int i = 0; i < 200; i++) {
    int value = analogRead(sensorPin);
    float diff = value - offset;
    sum += (long)(diff * diff);   // ✅ avoid warning
    delay(1);
  }

  float rms = sqrt(sum / 200.0);
  float voltage = rms * (3.3 / 4095.0);

  current = voltage / 0.185;

  // Noise filtering
  if (current < 0.02) current = 0;
  if (current > 1.0) current = 0;

  power = 230.0 * current;

  // Smoothing filter
  smoothPower = (0.7 * smoothPower) + (0.3 * power);

  // Energy calculation
  if (millis() - lastTime >= 1000) {
    lastTime = millis();
    energy += (smoothPower / 1000.0) / 3600.0;
  }

  // Prediction
  float predictedCost = 0;
  if (smoothPower > 7) {
    float monthlyUnits = (smoothPower / 1000.0) * 24 * 30;
    predictedCost = monthlyUnits * rate;
  }

  // ================= LCD DISPLAY =================

  // Screen 1
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P:");
  lcd.print(smoothPower, 1);
  lcd.print("W ");

  lcd.print("I:");
  lcd.print(current, 2);
  lcd.print("A");

  lcd.setCursor(0, 1);
  lcd.print("Live Load");

  delay(2000);

  // Screen 2
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Energy:");
  lcd.print(energy, 3);
  lcd.print("kWh");

  lcd.setCursor(0, 1);
  lcd.print("Usage");

  delay(2000);

  // Screen 3
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Predicted Bill");

  lcd.setCursor(0, 1);
  lcd.print("Rs:");
  lcd.print(predictedCost, 0);

  delay(2000);

  // Serial Debug
  Serial.print("Current: ");
  Serial.print(current);
  Serial.print(" A | Power: ");
  Serial.print(smoothPower);
  Serial.print(" W | Energy: ");
  Serial.print(energy, 6);
  Serial.print(" kWh | Predicted Cost: ");
  Serial.println(predictedCost);
}