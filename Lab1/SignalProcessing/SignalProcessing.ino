#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_CS   26
#define BME_MOSI 19
#define BME_MISO 21
#define BME_SCK  5

Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

#define NUM_SAMPLES 5000
#define N_SMALL 50
#define N_LARGE 500
#define SAMPLE_MS 10

// Moving average buffers
float maBufSmall[N_SMALL] = {};
float maBufLarge[N_LARGE] = {};
float maSumSmall = 0;
float maSumLarge = 0;
int maHeadSmall = 0;
int maHeadLarge = 0;
int maCountSmall = 0;
int maCountLarge = 0;

// Moving median buffers
float medBufSmall[N_SMALL] = {};
float medBufLarge[N_LARGE] = {};
int medHeadSmall = 0;
int medHeadLarge = 0;
int medCountSmall = 0;
int medCountLarge = 0;
float sortBuf[N_LARGE];

// EMA state
float emaSmall = 0;
float emaLarge = 0;
const float alphaSmall = 2.0 / (N_SMALL + 1);
const float alphaLarge = 2.0 / (N_LARGE + 1);
bool firstSample = true;

// Stats for each method
float rawMin, rawMax, rawSum, rawSumSq;
float maSmMin, maSmMax, maSmSum, maSmSumSq;
float maLgMin, maLgMax, maLgSum, maLgSumSq;
float mmSmMin, mmSmMax, mmSmSum, mmSmSumSq;
float mmLgMin, mmLgMax, mmLgSum, mmLgSumSq;
float emSmMin, emSmMax, emSmSum, emSmSumSq;
float emLgMin, emLgMax, emLgSum, emLgSumSq;

int sampleCount = 0;
bool done = false;
unsigned long lastSampleTime = 0;

void updateStats(float val, float &mn, float &mx, float &sm, float &sq) {
  if (val < mn) mn = val;
  if (val > mx) mx = val;
  sm += val;
  sq += val * val;
}

void initStats(float &mn, float &mx, float &sm, float &sq) {
  mn = 1e9; mx = -1e9; sm = 0; sq = 0;
}

float getMedian(float* buf, int head, int count, int size) {
  // Copy window into sort buffer
  for (int i = 0; i < count; i++) {
    sortBuf[i] = buf[(head - count + i + size) % size];
  }
  // Simple insertion sort
  for (int i = 1; i < count; i++) {
    float key = sortBuf[i];
    int j = i - 1;
    while (j >= 0 && sortBuf[j] > key) {
      sortBuf[j + 1] = sortBuf[j];
      j--;
    }
    sortBuf[j + 1] = key;
  }
  return sortBuf[count / 2];
}

void setup() {
  Serial.begin(115200);

  if (!bme.begin()) {
    Serial.println("BME688 not found - check SPI wiring!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_1X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_0);
  bme.setHumidityOversampling(BME680_OS_NONE);
  bme.setPressureOversampling(BME680_OS_NONE);
  bme.setGasHeater(0, 0);

  // Initialise all stats
  initStats(rawMin, rawMax, rawSum, rawSumSq);
  initStats(maSmMin, maSmMax, maSmSum, maSmSumSq);
  initStats(maLgMin, maLgMax, maLgSum, maLgSumSq);
  initStats(mmSmMin, mmSmMax, mmSmSum, mmSmSumSq);
  initStats(mmLgMin, mmLgMax, mmLgSum, mmLgSumSq);
  initStats(emSmMin, emSmMax, emSmSum, emSmSumSq);
  initStats(emLgMin, emLgMax, emLgSum, emLgSumSq);

  Serial.println("Starting 5000 sample collection from BME688 over SPI...");
  Serial.println("sample,raw,ma50,ma500,med50,med500,ema50,ema500");
}

void loop() {
  if (done) return;

  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_MS) return;
  lastSampleTime = now;

  if (!bme.performReading()) {
    Serial.println("Read failed");
    return;
  }

  float raw = bme.temperature;

  // Init EMA on first sample
  if (firstSample) {
    emaSmall = raw;
    emaLarge = raw;
    firstSample = false;
  }

  // Moving average (small)
  maSumSmall -= maBufSmall[maHeadSmall];
  maBufSmall[maHeadSmall] = raw;
  maSumSmall += raw;
  maHeadSmall = (maHeadSmall + 1) % N_SMALL;
  if (maCountSmall < N_SMALL) maCountSmall++;
  float ma50 = maSumSmall / maCountSmall;

  // Moving average (large)
  maSumLarge -= maBufLarge[maHeadLarge];
  maBufLarge[maHeadLarge] = raw;
  maSumLarge += raw;
  maHeadLarge = (maHeadLarge + 1) % N_LARGE;
  if (maCountLarge < N_LARGE) maCountLarge++;
  float ma500 = maSumLarge / maCountLarge;

  // Moving median (small)
  medBufSmall[medHeadSmall] = raw;
  medHeadSmall = (medHeadSmall + 1) % N_SMALL;
  if (medCountSmall < N_SMALL) medCountSmall++;
  float mm50 = getMedian(medBufSmall, medHeadSmall, medCountSmall, N_SMALL);

  // Moving median (large)
  medBufLarge[medHeadLarge] = raw;
  medHeadLarge = (medHeadLarge + 1) % N_LARGE;
  if (medCountLarge < N_LARGE) medCountLarge++;
  float mm500 = getMedian(medBufLarge, medHeadLarge, medCountLarge, N_LARGE);

  // EMA
  emaSmall = alphaSmall * raw + (1.0 - alphaSmall) * emaSmall;
  emaLarge = alphaLarge * raw + (1.0 - alphaLarge) * emaLarge;

  // Update stats for each method
  updateStats(raw,    rawMin,  rawMax,  rawSum,  rawSumSq);
  updateStats(ma50,   maSmMin, maSmMax, maSmSum, maSmSumSq);
  updateStats(ma500,  maLgMin, maLgMax, maLgSum, maLgSumSq);
  updateStats(mm50,   mmSmMin, mmSmMax, mmSmSum, mmSmSumSq);
  updateStats(mm500,  mmLgMin, mmLgMax, mmLgSum, mmLgSumSq);
  updateStats(emaSmall, emSmMin, emSmMax, emSmSum, emSmSumSq);
  updateStats(emaLarge, emLgMin, emLgMax, emLgSum, emLgSumSq);

  // CSV output for plotting
  Serial.printf("%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
    sampleCount, raw, ma50, ma500, mm50, mm500, emaSmall, emaLarge);

  sampleCount++;

  if (sampleCount >= NUM_SAMPLES) {
    done = true;

    float n = NUM_SAMPLES;
    Serial.println("\n=== RESULTS (5000 samples, BME688 SPI) ===");
    Serial.println("Method             |   Min   |   Max   |  Mean   | StdDev");
    Serial.println("-------------------|---------|---------|---------|-------");

    // Mean = sum/n, StdDev = sqrt((sumSq - sum^2/n) / (n-1))
    float methods[7][4] = {
      {rawMin,  rawMax,  rawSum,  rawSumSq},
      {maSmMin, maSmMax, maSmSum, maSmSumSq},
      {maLgMin, maLgMax, maLgSum, maLgSumSq},
      {mmSmMin, mmSmMax, mmSmSum, mmSmSumSq},
      {mmLgMin, mmLgMax, mmLgSum, mmLgSumSq},
      {emSmMin, emSmMax, emSmSum, emSmSumSq},
      {emLgMin, emLgMax, emLgSum, emLgSumSq},
    };
    const char* labels[7] = {
      "Raw", "MovAvg N=50", "MovAvg N=500",
      "MovMed N=50", "MovMed N=500",
      "EMA a=2/51", "EMA a=2/501"
    };

    for (int i = 0; i < 7; i++) {
      float mean = methods[i][2] / n;
      float stddev = sqrtf((methods[i][3] - methods[i][2]*methods[i][2]/n) / (n-1));
      Serial.printf("%-18s | %7.3f | %7.3f | %7.3f | %7.4f\n",
        labels[i], methods[i][0], methods[i][1], mean, stddev);
    }
  }
}