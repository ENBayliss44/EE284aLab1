#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// ---- Pins ----
#define ADC_PIN    A2
#define LED_PIN    13
#define BUTTON_PIN 38

#define BME_SCK 5
#define BME_MISO 21
#define BME_MOSI 19
#define BME_CS 26

Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

// ---- EMA config ----
const float ALPHA = 2.0 / (50 + 1);
float emaTMP36 = 0;
float emaBME   = 0;

// ---- Threshold ----
const float DELTA_THRESHOLD = 1.5;

// ---- Timing ----
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_MS = 200;

// ---- State ----
bool started    = false;
bool lastButton = HIGH;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // Set attenuation once at startup and leave it
  analogSetPinAttenuation(ADC_PIN, ADC_6db);
  delay(500);

  pinMode(LED_PIN,    OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  if (!bme.begin()) {
    Serial.println("BME688 not found - check SPI wiring!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_1X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_0);
  bme.setHumidityOversampling(BME680_OS_NONE);
  bme.setPressureOversampling(BME680_OS_NONE);
  bme.setGasHeater(0, 0);

  Serial.println("Ready - press button to start.");
}

void loop() {
  // ---- Button debounce ----
  bool currentButton = digitalRead(BUTTON_PIN);
  if (lastButton == HIGH && currentButton == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      started = !started;
      if (started) {
        // Flush ADC before seeding EMA
        for (int i = 0; i < 5; i++) { analogRead(ADC_PIN); delay(10); }

        // Seed EMA from stable reading
        int dn = analogRead(ADC_PIN);
        emaTMP36 = ((dn / 4095.0) * 2.1 - 0.5) / 0.010;

        bme.performReading();
        emaBME = bme.temperature;

        Serial.printf("Seeded: TMP36=%.2fC  BME=%.2fC\n", emaTMP36, emaBME);
        Serial.println("Started.");
      } else {
        digitalWrite(LED_PIN, LOW);
        Serial.println("Stopped.");
      }
    }
  }
  lastButton = currentButton;

  if (!started) return;

  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_MS) return;
  lastSampleTime = now;

  // ---- Read TMP36 ----
  int dn = analogRead(ADC_PIN);
  float voltage = (dn / 4095.0) * 2.1;
  float rawTMP36 = (voltage - 0.5) / 0.010;

  // ---- Read BME688 ----
  if (!bme.performReading()) {
    Serial.println("BME688 read failed");
    return;
  }
  float rawBME = bme.temperature;

  // ---- EMA filter ----
  emaTMP36 = ALPHA * rawTMP36 + (1.0 - ALPHA) * emaTMP36;
  emaBME   = ALPHA * rawBME   + (1.0 - ALPHA) * emaBME;

  // ---- Compare and actuate ----
  float delta = emaTMP36 - emaBME;
  if (delta < 0) delta = -delta;

  bool ledOn = delta > DELTA_THRESHOLD;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  Serial.printf("TMP36: %.2fC  BME688: %.2fC  Delta: %.2fC  LED: %s\n",
    emaTMP36, emaBME, delta, ledOn ? "ON" : "OFF");
}