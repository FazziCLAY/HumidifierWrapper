#include <LiquidCrystal.h>
#include <EncButton.h>
#include "DataArray.h"



// ========= SETTINGS =========
#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 2
#define DISPLAY_RS 11
#define DISPLAY_EN 12
#define DISPLAY_D4 10
#define DISPLAY_D5 9
#define DISPLAY_D6 8
#define DISPLAY_D7 7
#define DISPLAY_LED 6

#define ENCODER_S1 2
#define ENCODER_S2 3
#define ENCODER_KEY 4

#define WATER_MIN 370
#define DEFAULT_HUM_MODE 1
#define DEFAULT_HUM_LED_MODE 0
// ========= SETTINGS =========


LiquidCrystal lcd(DISPLAY_RS, DISPLAY_EN, DISPLAY_D4, DISPLAY_D5, DISPLAY_D6, DISPLAY_D7);
EncButton enc(ENCODER_S1, ENCODER_S2, ENCODER_KEY);
DataArray waterSensor(30);

// 0 - off
// 1 - RGB
// 2 - white hue
// 3 - static white
static int humidifierLedState = 0;
static int humidifierLedStateGoal = DEFAULT_HUM_LED_MODE; // -1 - no goal; goal success

// 0 - off
// 1 - always on
// 2 - 50/50
static int humidifierState = 0;
static int humidifierStateGoal = DEFAULT_HUM_MODE; // goal success

static uint32_t humidifierButtonReleseAt = -1;
static int humidifierButtonReleseNewLedState = -1;
static int humidifierButtonReleseNewState = -1;

void setup() {
  Serial.begin(9600);
  analogReference(DEFAULT);

  pinMode(A0, OUTPUT);


  pinMode(DISPLAY_LED, OUTPUT);
  digitalWrite(DISPLAY_LED, true);


  pinMode(A5, OUTPUT); // power
  analogWrite(A5, 1023); // power enable

  pinMode(A6, INPUT); // water sensor
  pinMode(A3, OUTPUT); // button

  enc.init(INPUT_PULLUP, INPUT_PULLUP, LOW);
  enc.setEncType(EB_STEP4_LOW);
  lcd.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void pressButton(bool hold) {
  Serial.print("pressButton "); Serial.println(hold);
  pinMode(A2, OUTPUT); // button
  analogWrite(A2, 0); // Подаем землю (LOW) на A7

  int delay = 70;
  if (hold) {
    delay = 1500;
    humidifierButtonReleseNewLedState = (humidifierLedState + 1) % 4;
  } else {
    delay = 70;
    humidifierButtonReleseNewState = (humidifierState + 1) % 3;
  }
  humidifierButtonReleseAt = millis() + delay;
}


void loopPostPressButton() {
  if (millis () >= humidifierButtonReleseAt && humidifierButtonReleseAt > 0) {
    pinMode(A2 , INPUT); // Смена режима на INPUT, чтобы «отпустить» пин
    if (humidifierButtonReleseNewLedState != -1) {
      humidifierLedState = humidifierButtonReleseNewLedState;
    }

    if (humidifierButtonReleseNewState != -1) {
      humidifierState = humidifierButtonReleseNewState;

      if (humidifierState == 0) {
        humidifierLedState = 0;
      }
      if (humidifierState == 1) {
        humidifierLedState = 1;
      }
    }

    humidifierButtonReleseNewLedState = -1;
    humidifierButtonReleseNewState = -1;

    if (millis() - humidifierButtonReleseAt > 500) {
      humidifierButtonReleseAt = -1;
    }
  }

}

bool isNoWater() {
  return getWaterSensor() < WATER_MIN;
}

bool isWater() {
  return !isNoWater();
}

bool isGoals() {
  return humidifierStateGoal != -1 || humidifierLedStateGoal != -1;
}

int getWaterSensor() {
  int appendix = 0;
  if (humidifierState > 0) {
    appendix = 120;
  }
  return waterSensor.get() + appendix;
}

static boolean updateForNoSleep = false;

void loop() {
  enc.tick();
  waterSensor.put(analogRead(A6));
  loopPostPressButton();

  static uint32_t tmr = 0;
  static bool stateOfHumidifier = true;
  uint32_t tmrTime = (uint32_t)millis() - (uint32_t)tmr;
  static uint32_t l = (uint32_t)1000 * (uint32_t)60 * (uint32_t)60;
  if (isWater() && (tmrTime >= l)) {
    if (!isGoals()) {
      humidifierStateGoal = stateOfHumidifier ? DEFAULT_HUM_MODE : 0;
      humidifierLedStateGoal = DEFAULT_HUM_LED_MODE;
      tmr = millis();
      stateOfHumidifier = !stateOfHumidifier;
    }
  }

  if (isNoWater() && humidifierState != 0) {
    humidifierStateGoal = 0;
  }

  if (isNoWater()) {
      static uint32_t tmrLedBlink = millis();
      static bool b = false;
      if (millis() - tmrLedBlink > 500) {
        digitalWrite(DISPLAY_LED, b);
        tmrLedBlink = millis();
        b = !b;
      }   
  } else {
    digitalWrite(DISPLAY_LED, false);
  }

  if (humidifierButtonReleseAt == -1) {
    if ((humidifierStateGoal != -1) && (humidifierState != humidifierStateGoal)) {
      pressButton(false);
    }
    if (humidifierStateGoal == humidifierState) {
      humidifierStateGoal = -1;
    }
  }

  if (humidifierButtonReleseAt == -1) {
    if ((humidifierLedStateGoal != -1) && (humidifierLedState != humidifierLedStateGoal)) {
      pressButton(true);
    }
    if (humidifierLedState == humidifierLedStateGoal) {
      humidifierLedStateGoal = -1;
    }
  }


  lcd.setCursor(1, 0);
  lcd.print("---");
  lcd.setCursor(1, 0);
  lcd.print(getWaterSensor());





  lcd.setCursor(1, 1);
  lcd.print("H:");
  lcd.print(humidifierState);
  lcd.print("/");
  lcd.print(humidifierStateGoal);

  lcd.print(" L:");
  lcd.print(humidifierLedState);
  lcd.print("/");
  lcd.print(humidifierLedStateGoal);


  int i = min(max(0, getWaterSensor() - 300), 230)*4; // [0; 240] USSR
  //Serial.println(i);

  lcd.setCursor(5, 0);
  lcd.print("---");
  lcd.setCursor(5, 0);
  lcd.print(max(0, getWaterSensor() - 300) / 3);
  lcd.print("%  ");

  analogWrite(A3, 140);
  delayMicroseconds(i);
  analogWrite(A3, 120);
  delayMicroseconds(1000 - i);

  //analogWrite(A3, i);

  //delay(10);
}
