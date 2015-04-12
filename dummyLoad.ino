/*

*/


#include <LiquidCrystal.h>

// Setup LCD Pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pin 10 is used to switch the load on/off
const int controlPin = 10;

// Reference Voltage (mV)
const float vRef = 1235.0;

// Multiplier to scale up from voltage divider
const float vMul = (10000.0 + 3300.0)/3300.0;

// Analog pins for potentiometer and load voltage
const int potPin = 2;
const int loadPin = 3;

// Number of samples
const int nSamples = 30;
const float sampleMul = 1.0 / (float) nSamples;

// Track state
byte state = 0;

// keep track of mAh and mWh;
float cummulative_mAh = 0.0, cummulative_mWh = 0.0;

// Track output times
int lastOutput = 0;
long start;

// When to finish
int batteryCuttoff = 3000;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Setup our reference voltage
  analogReference(EXTERNAL);

  // Enable load control and force off
  pinMode(controlPin, OUTPUT);
  digitalWrite(controlPin, HIGH);

  // Setup serial port
  Serial.begin(115200);

  // Output initial message
  Serial.println(F("# Dummy Load v0.3"));
  lcd.print(F("Dummy Load v0.3"));

  // Initial delay to show the above message
  delay(1000);
}

void display(int result) {
  if ( result < 1000 ) lcd.print(" ");
  if ( result < 100 ) lcd.print(" ");
  if ( result < 10 ) lcd.print(" ");
  lcd.print(result);
}

void mV(int result) {
  display(result);
  lcd.print("mV");
}

void mA(int result) {
  display(result);
  lcd.print("mA");
}

void mW(int result) {
  display(result);
  lcd.print("mW");
}

void mAh(int result) {
  display(result);
  lcd.print("mAh");
}

void mWh(int result) {
  display(result);
  lcd.print("mWh");
}

float sample(int pin) {
  long sum = 0;
  analogRead(pin);
  delay(25);
  for (int i = 0; i < nSamples; i++) {
    sum += analogRead(pin);
  }
  return (float) sum * sampleMul;
}

float milliAmps() {
  float fval = sample(potPin);
  return vRef*fval/1023.0;
}

int loadVoltage() {
  float fval = milliAmps();
  if (fval < 0.01) fval = 0.0;
  return int(fval+0.5);
}

float milliVolts() {
  float fval = sample(loadPin);
  return vMul*vRef*fval/1023.0;
}

int batteryVoltage() {
  float fval = milliVolts();
  return int(fval+0.5);
}

void separator() {
  Serial.print(" ");
}

void loop() {

  unsigned long now = millis();
  int sec = now / 1000;

  if ( state == 0 ) {
    if ( batteryVoltage() > 10 ) {
      lcd.setCursor(0, 1);
      lcd.print(F("Remove battery!"));
      delay(1000);
      state = 1;
    } else {
      state = 2;
    }
    return;
  }

  if ( state == 1 ) {
    // Waiting for battery to be removed
    if ( batteryVoltage() < 10 ) {
      state = 2;
    }
    return;
  }

  if ( state == 2 ) {
    // Prepare to set current
    //startup, no battery so all is fine!
    lcd.setCursor(0, 0);
    lcd.print("Set current then");
    lcd.setCursor(0, 1);
    lcd.print("add batt: ");
    // Enable setting of current
    digitalWrite(controlPin, LOW);
    state = 3;
    return;
  }

  if ( state == 3 ) {
    // Waiting for battery to appear, refresh mA display...
    lcd.setCursor(10, 1);
    mA(loadVoltage());
    delay(250);
    int bv = batteryVoltage();
    if ( bv > 10 ) {
      state = 4;
      // Signal start of run 
      Serial.println(F("# Start"));
      // Prepare LCD display
      lcd.clear();

      // Detect battery type and set threshold battery voltage
      if ( bv > 3000 ) {
        lcd.print("Li-ion");
        batteryCuttoff = 3000;
      } else {
        lcd.print("Other ");
        batteryCuttoff = 800;
      }
    }
    start = sec;
    return;
  }

  if ( state == 4 ) {
    // Normal run time...
    if ( sec != lastOutput ) {
      lastOutput = sec;

      float mA1 = milliAmps();
      cummulative_mAh += mA1 / 3600.0;
      int imAh = int(cummulative_mAh+0.5);

      int bv = batteryVoltage();
      float mW1 = mA1 * bv / 1000.0;
      cummulative_mWh += mW1 / 3600.0;
      if ( bv < batteryCuttoff ) {
        state = 5;
        // Cutoff reached
        digitalWrite(controlPin, HIGH);
      }

      lcd.setCursor(9, 0);
      mV(bv);
      lcd.setCursor(0, 1);
      mAh(imAh);
      lcd.setCursor(9, 1);
      mA(int(mA1+0.5));

      int mW = int(mA1*bv/1000.0+0.5);
      Serial.print(sec-start+1);
      separator();
      Serial.print(bv);
      separator();
      Serial.print(cummulative_mAh);
      separator();
      Serial.println(cummulative_mWh);
      separator();
    }
    delay(100);
    return;
  }

  if ( state == 5 ) {
    lcd.clear();
    lcd.print("Done @ ");
    mV(batteryVoltage());
    lcd.setCursor(0,1);
    mAh(int(cummulative_mAh+0.5));
    lcd.print("; ");
    mWh(int(cummulative_mWh+0.5));
    state = 99;
  }

}
