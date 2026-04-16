#define LED1_PIN 13
#define LED2_PIN 12
#define LED3_PIN 27

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  Serial.println("Enter 3-bit code to control LEDs (e.g. 101 = LED1 on, LED2 off, LED3 on):");
}

void loop() {
  if (Serial.available() >= 3) {
    char b1 = Serial.read();
    char b2 = Serial.read();
    char b3 = Serial.read();

    // flush trailing newline
    while (Serial.available()) Serial.read();

    if ((b1 == '0' || b1 == '1') &&
        (b2 == '0' || b2 == '1') &&
        (b3 == '0' || b3 == '1')) {

      digitalWrite(LED1_PIN, b1 == '1' ? HIGH : LOW);
      digitalWrite(LED2_PIN, b2 == '1' ? HIGH : LOW);
      digitalWrite(LED3_PIN, b3 == '1' ? HIGH : LOW);

      Serial.printf("LED1: %s  LED2: %s  LED3: %s\n",
        b1 == '1' ? "ON" : "OFF",
        b2 == '1' ? "ON" : "OFF",
        b3 == '1' ? "ON" : "OFF");

    } else {
      Serial.println("Invalid input - enter exactly 3 bits e.g. 101");
    }
  }
}