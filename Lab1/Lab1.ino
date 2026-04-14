#define ADC_PIN A2
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


void setup() {
  // Some boards work best if we also make a serial connection
  Serial.begin(115200);

  // set LED to be an output pin
  pinMode(led, OUTPUT);

  analogReadResolution(12);

  analogSetPinAttenuation(ADC_PIN, ADC_6db); // lock attenuation
  delay(50);
}

void loop() {
  unsigned long currentTime = millis();  

  if(currentTime - lastAdcTime >= adcInterval) {
    lastAdcTime = currentTime;
    mvRead = analogRead(ADC_PIN);

    celciusRead = DvDc*(static_cast<float>(mvRead) - MvOffset);

    // if(celciusRead > 25) {
    //   isAboveTwentyFive = true;
    // } else {
    //   isAboveTwentyFive = false
    // }


  Serial.printf("The raw ADC value is %u, which converts to %.2f V. The temperature is %s 23C, in fact it is %.2f C\n", 
              mvRead, 
              mvRead * 1000, 
              (celciusRead > 25) ? "above" : "below", 
              celciusRead);
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
