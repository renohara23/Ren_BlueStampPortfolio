#include <LiquidCrystal.h>
#include <PulseSensorPlayground.h>
const int BUZZER = 5;
const int GSR = A2;
const int LED = 4;
const int BUTTON = 2;
const int PulsePin = A0;
PulseSensorPlayground pulseSensor;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int sensorValue = 0;
int bpm = 0;
float ema = 0;
float baselineEMA = 0;
int baselineBPM = 0;
bool baselineSet = false;
bool collecting = false;
bool calibrating = false;
bool questionAsked = false;
long sumGSR = 0;
long sumBPM = 0;
int count = 0;
float alphaFast = 0.1;
float alphaSlow = 0.05;
int sensitivity = 1;  
unsigned long buttonPressTime = 0;
const unsigned long preCalDelay = 15000;
const unsigned long calDuration = 20000;

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED, LOW);

  lcd.begin(16, 2);
  lcd.print("Press to Calib");

  pulseSensor.analogInput(PulsePin);
  pulseSensor.setThreshold(550);
  pulseSensor.begin();
}

void loop() {
  sensorValue = analogRead(GSR);
  bpm = pulseSensor.getBeatsPerMinute();

  
  if (!baselineSet && !collecting && digitalRead(BUTTON) == LOW) {
    collecting = true;
    buttonPressTime = millis();
    lcd.clear();
    lcd.print("Wait 15 sec...");
    delay(500);
  }
  if (collecting && !calibrating && millis() - buttonPressTime < preCalDelay) {
    delay(10);
    return;
  }
  if (collecting && !calibrating && millis() - buttonPressTime >= preCalDelay) {
    calibrating = true;
    sumGSR = 0;
    sumBPM = 0;
    count = 0;
    buttonPressTime = millis();
    lcd.clear();
    lcd.print("Calibrating...");
    delay(10);
  }
  if (calibrating && millis() - buttonPressTime < calDuration) {
    sumGSR += sensorValue;
    sumBPM += bpm;
    count++;
    delay(10);
    return;
  }
  if (calibrating && millis() - buttonPressTime >= calDuration) {
    float avgGSR = sumGSR / (float)count;
    baselineEMA = avgGSR;
    ema = avgGSR;
    baselineBPM = sumBPM / count;

    baselineSet = true;
    collecting = false;
    calibrating = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Baseline Set");
    lcd.setCursor(0, 1);
    lcd.print("Avg GSR: ");
    lcd.print((int)avgGSR);
    delay(2000);
    lcd.clear();
  }
  if (baselineSet) {
    
    ema = alphaFast * sensorValue + (1 - alphaFast) * ema;
    baselineEMA = alphaSlow * ema + (1 - alphaSlow) * baselineEMA;

    
    if (digitalRead(BUTTON) == LOW && !questionAsked) {
      baselineEMA = 0.9 * baselineEMA + 0.1 * ema;
      baselineBPM = 0.9 * baselineBPM + 0.1 * bpm;
      questionAsked = true;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("New Q Incoming");
      lcd.setCursor(0, 1);
      lcd.print("Recalibrating...");
      delay(1500);
      lcd.clear();
    }
    if (digitalRead(BUTTON) == HIGH) {
      questionAsked = false;
    }

    
    float gsrScore = (ema - baselineEMA) / sensitivity;
    float bpmScore = (bpm - baselineBPM) / sensitivity;
    float totalScore = max(0, gsrScore) + max(0, bpmScore);  // Ignore negatives
    bool lieDetected = totalScore > 2.0;

    
    lcd.setCursor(0, 0);
    lcd.print("GSR:");
    lcd.print((int)ema);
    lcd.print(" HR:");
    lcd.print(bpm);
    lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print("Score:");
    lcd.print(totalScore, 1);
    if (lieDetected) {
      lcd.print(" Lie  ");
      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED, HIGH);
    } else {
      lcd.print("     ");
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED, LOW);
    }

    
    Serial.print("EMA:");
    Serial.print(ema);
    Serial.print(" BPM:");
    Serial.print(bpm);
    Serial.print(" Score:");
    Serial.println(totalScore);
    delay(10);
  }
}
