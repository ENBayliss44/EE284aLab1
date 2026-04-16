const int beamPin  = 27;
const int servoPin = 13;

const int pwmResolution = 16;
const int pwmFreq       = 50;

uint32_t minPulse = 1550;
uint32_t midPulse = 1650;
uint32_t maxPulse = 1750;

const float rpmTarget = 60.0;
const float Kp        = 2.0;  

uint32_t currentPulse = 0;

volatile unsigned long lastEdgeTime = 0;
volatile unsigned long edgeInterval = 0;
volatile bool newEdge = false;

void IRAM_ATTR beamISR() {
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

uint32_t clampPulse(uint32_t p) {
  if (p < minPulse) return minPulse;
  if (p > maxPulse) return maxPulse;
  return p;
}

void setup() {
  Serial.begin(115200);
  pinMode(beamPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(beamPin), beamISR, RISING);

  ledcAttach(servoPin, pwmFreq, pwmResolution);
  delay(100);

  currentPulse = midPulse;
  ledcWrite(servoPin, dutyFromUs(currentPulse));

  Serial.println("Closed-loop RPM control. Target: 60 RPM");
  Serial.println("RPM,Pulse");
}

void loop() {
  if (!newEdge) return;
  newEdge = false;

  unsigned long interval = edgeInterval;
  if (interval == 0) return;

  float rpm = 60.0 * 1e6 / (float)interval;

  // Proportional controller
  float error      = rpmTarget - rpm;
  int   adjustment = (int)(Kp * error);
  currentPulse     = clampPulse(currentPulse + adjustment);
  ledcWrite(servoPin, dutyFromUs(currentPulse));

  Serial.printf("%.1f,%u\n", rpm, currentPulse);
}