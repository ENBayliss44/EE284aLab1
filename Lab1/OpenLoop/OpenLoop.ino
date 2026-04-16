const int beamPin  = 27;
const int servoPin = 13;

const int pwmResolution = 16;
const int pwmFreq       = 50;

uint32_t minPulse = 1550;
uint32_t midPulse = 1650;
uint32_t maxPulse = 1750;

volatile unsigned long lastEdgeTime = 0;
volatile unsigned long edgeInterval = 0;
volatile bool newEdge = false;

void IRAM_ATTR beamISR() {
  // Only act on the rising edge (beam unblocked)
  if (digitalRead(beamPin) == LOW) return;
  
  unsigned long now = micros();
  if (lastEdgeTime > 0 && (now - lastEdgeTime) < 50000) return;
  if (lastEdgeTime > 0) {
    edgeInterval = now - lastEdgeTime;
    newEdge = true;
  }
  lastEdgeTime = now;
}

uint32_t dutyFromUs(uint32_t pulseUs) {
  const uint32_t maxDuty = (1UL << pwmResolution) - 1;
  return (pulseUs * maxDuty) / 20000UL;
}

void setup() {
  Serial.begin(115200);
  pinMode(beamPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(beamPin), beamISR, RISING);

  ledcAttach(servoPin, pwmFreq, pwmResolution);
  delay(100);

  ledcWrite(servoPin, dutyFromUs(maxPulse));
  Serial.println("Running at minPulse (1550us)");
  Serial.println("RPM readings:");
}

void loop() {
  if (!newEdge) return;
  newEdge = false;

  unsigned long interval = edgeInterval;
  if (interval == 0) return;

  float rpm = 60.0 * 1e6 / (float)interval;
  Serial.printf("Interval: %lu us  |  RPM: %.1f\n", interval, rpm);
}