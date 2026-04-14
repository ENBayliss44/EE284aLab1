#define ADC_PIN A2
#define DURATION_MS 60000
int led = LED_BUILTIN;

/* LED Variables */
const unsigned long IntervalHigh = 500; // ms
const unsigned long IntervalLow = 500; // ms
unsigned long ledInterval = 0;
unsigned long lastChangeTime = 0;
bool ledIsHigh = false;

/* ADC Variables*/
unsigned long lastAdcTime = 0;
const unsigned long adcInterval = 500; // ms
uint16_t mvRead = 0;
float celciusRead = 0;

const float DvDc = 100;
const float MvOffset = 0.5;

float tempMax = -1e9f;
float tempMin =  1e9f;
bool  done    = false;

unsigned long startTime = 0;


void setup() {
  // Some boards work best if we also make a serial connection
  Serial.begin(115200);

  // set LED to be an output pin
  pinMode(led, OUTPUT);

  analogReadResolution(12);

  startTime = millis();

  analogSetPinAttenuation(ADC_PIN, ADC_6db); // lock attenuation
  delay(50);
}

void loop() {
  if (done) return;

  unsigned long currentTime = millis();  

  if(currentTime - lastAdcTime >= adcInterval) {
    analogSetPinAttenuation(ADC_PIN, ADC_6db);
    delay(50);
    analogRead(ADC_PIN); // dummy read, discard result
    
    lastAdcTime = currentTime;
    mvRead = analogRead(ADC_PIN);

    float voltage = (mvRead / 4095.0f) * 2.1f;
    celciusRead = (voltage - 0.5f) / 0.010f;

    if (celciusRead > tempMax) tempMax = celciusRead;
    if (celciusRead < tempMin) tempMin = celciusRead;

    Serial.printf(
      "The raw ADC value is %u, which converts to %.4f V. "
      "The temperature is %s 23C, in fact it is %.2f C\n",
      mvRead,
      voltage,
      (celciusRead > 23.0f) ? "above" : "below",
      celciusRead
    );
  }

  if (currentTime - startTime >= DURATION_MS) {
    done = true;
    Serial.println("\n=== 60 SECOND SUMMARY ===");
    Serial.printf("Max temp : %.2f C\n", tempMax);
    Serial.printf("Min temp : %.2f C\n", tempMin);
    Serial.printf("Delta    : %.2f C\n", tempMax - tempMin);
  }

  ledRoutine(currentTime);

}

void ledRoutine(unsigned long currentTime)
{
  // Non-blocking led routing
  if(currentTime - lastChangeTime >= ledInterval) {
    lastChangeTime = currentTime; // Reset timer

    if(ledIsHigh) {
      // Turn off the LED
      digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
      ledInterval = IntervalLow; // set wait time for the "OFF" phase
      ledIsHigh = false;
    } else {
      // Turn on the LED
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      ledInterval = IntervalHigh;
      ledIsHigh = true;
    }
  } 
}
