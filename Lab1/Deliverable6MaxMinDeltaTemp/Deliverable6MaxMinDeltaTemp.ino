#define ADC_PIN A2
#define DURATION_MS 60000
#define SAMPLE_MS   500

float tempMax = -1e9f;
float tempMin =  1e9f;
bool  done    = false;

unsigned long startTime = 0;
unsigned long lastSampleTime = 0;

float adcToTemp(uint16_t dn) {
  float v = (dn / 4095.0f) * 2.1f;
  return (v - 0.5f) / 0.010f;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_6db);
  delay(50);
  startTime = millis();
  Serial.println("Sampling for 60 seconds...");
}

void loop() {
  if (done) return;

  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_MS) {
    analogSetPinAttenuation(ADC_PIN, ADC_6db);
    delay(50);
    lastSampleTime = now;

    uint16_t dn = analogRead(ADC_PIN);
    float temp = adcToTemp(dn);

    if (temp > tempMax) tempMax = temp;
    if (temp < tempMin) tempMin = temp;

    // Print progress every sample to see it working
    int secsLeft = (DURATION_MS - (int)(now - startTime)) / 1000;
    Serial.printf("T = %.2f C  |  Max = %.2f  Min = %.2f  |  %ds remaining\n",
                  temp, tempMax, tempMin, secsLeft);
  }

  if (now - startTime >= DURATION_MS) {
    done = true;
    Serial.println("\n=== 60 SECOND SUMMARY ===");
    Serial.printf("Max temp : %.2f C\n", tempMax);
    Serial.printf("Min temp : %.2f C\n", tempMin);
    Serial.printf("Delta    : %.2f C\n", tempMax - tempMin);
  }
}