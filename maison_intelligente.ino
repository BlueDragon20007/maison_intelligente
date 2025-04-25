#include <LCD_I2C.h>
#include <AccelStepper.h>
#include <HCSR04.h>
#include "SSD1306.h"
#define TRIGGER_PIN 9
#define ECHO_PIN 10
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
#define IN_1 3
#define IN_2 4
#define IN_3 5
#define IN_4 6
#define BUZZER_PIN 8
#define LED_RED 13
#define LED_GREEN 12
#define LED_BLUE 11
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// #define OLED_RESET -1
// #define SCREEN_ADDRESS 0x3C

SSD1306 display(SCREEN_ADDRESS);
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

LCD_I2C lcd(0x27, 16, 2);

AccelStepper myStepper(AccelStepper::FULL4WIRE, IN_1, IN_3, IN_2, IN_4);

enum State {
  alarm,
  tooClose,
  automatic,
  tooFar
};

enum Commands {
  NONE,
  G_DIST,
  CFG_ALM,
  CFG_LIM_INF,
  CFG_LIM_SUP,
  UNKNOWN
};

Commands command = NONE;

State state;

unsigned long currentTime;
unsigned long lcdLastTime = 0;
unsigned long serialLastTime = 0;
unsigned long lastAlarmTriggerTime = 0;

float stepsByDegree = 2038.0 / 360.0;
int angle = 0;
int currentAngle = 0;
int steps = 0;

int distance = 40;
int newDistance = 0;
int minDistance = 30;
int maxDistance = 60;
int alarmTriggerDistance = 15;
int minDegree = 10;
int maxDegree = 170;

int stepperMaxSpeed = 1000;
int stepperAcceleration = 1000;
int stepperSpeed = 1000;

int lcdDelay = 100;
int serialDelay = 100;
int distanceDelay = 50;
int alarmBlinkDelay = 250;
int startingPrintTime = 2000;

bool ledState = false;

String input = "";

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  myStepper.setMaxSpeed(stepperMaxSpeed);          // Vitesse max en pas/seconde
  myStepper.setAcceleration(stepperAcceleration);  // Accélération en pas/seconde²
  myStepper.setSpeed(stepperSpeed);                // Vitesse constante en pas/seconde

  display.begin();

  lcd.begin();
  lcd.backlight();
  lcd.print("6307713");
  lcd.setCursor(0, 1);
  lcd.print("Labo 4B");
  delay(startingPrintTime);
  lcd.clear();
}

void loop() {
  currentTime = millis();

  display.update();

  handleSerialCommands();

  getDistance();

  stateManager();

  runAlarm();

  runMotor();

  lcdTask();

  //serialTask();
}

void manageCommand(String input) {
  if (input == "gDist" || input == "g_dist") {
    command = G_DIST;
  } else if (input.startsWith("cfg;alm;")) {
    command = CFG_ALM;
  } else if (input.startsWith("cfg;lim_inf;")) {
    command = CFG_LIM_INF;
  } else if (input.startsWith("cfg;lim_sup;")) {
    command = CFG_LIM_SUP;
  } else {
    command = UNKNOWN;
  }
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    input = Serial.readStringUntil('\n');
    input.trim();
    //Serial.println("You typed: " + input);

    manageCommand(input);

    switch (command) {
      case G_DIST:
        Serial.println(distance);
        display.displaySuccess();
        break;

      case CFG_ALM: {
        int val = input.substring(input.lastIndexOf(';') + 1).toInt();
        alarmTriggerDistance = val;
        display.displaySuccess();
        break;
      }

      case CFG_LIM_INF: {
        int val = input.substring(input.lastIndexOf(';') + 1).toInt();
        if (val >= maxDistance) {
          Serial.println("Erreur – Limite inférieure plus grande que limite supérieure");
          display.displayError();
        } else {
          minDistance = val;
          display.displaySuccess();
        }
        break;
      }

      case CFG_LIM_SUP: {
        int val = input.substring(input.lastIndexOf(';') + 1).toInt();
        if (val <= minDistance) {
          Serial.println("Erreur – Limite supérieure plus petite que limite inférieure");
          display.displayError();
        } else {
          maxDistance = val;
          display.displaySuccess();
        }
        break;
      }

      case UNKNOWN:
        display.displayUnknown();
        break;

      default:
        break;
    }

    input = "";
    command = NONE;
  }
}


void getDistance() {
  static unsigned long lastTime = 0;
  if (currentTime - lastTime >= distanceDelay) {
    lastTime = currentTime;
    newDistance = hc.dist();
    if (newDistance != 0) {
      distance = newDistance;
    }
  }
}

void stateManager() {
  if (distance <= alarmTriggerDistance) {
    state = alarm;
    lastAlarmTriggerTime = currentTime;
  } else if (state == alarm && (currentTime - lastAlarmTriggerTime < 3000)) {
    // Reste en état alarm
  } else if (distance < minDistance) {
    state = tooClose;
  } else if (distance > maxDistance) {
    state = tooFar;
  } else {
    state = automatic;
  }
}

void runAlarm() {
  static unsigned long lastTime = 0;
  if (state == alarm) {
    digitalWrite(BUZZER_PIN, HIGH);
    if (currentTime - lastTime >= alarmBlinkDelay) {
      lastTime = currentTime;
      ledState = !ledState;
    }
    if (ledState) {
      setColor(0, 0, 255);
    } else {
      setColor(255, 0, 0);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    setColor(0, 0, 0);
  }
}

void runMotor() {
  if (state == automatic) {
    angle = map(distance, minDistance, maxDistance, minDegree, maxDegree);
    steps = angle * stepsByDegree;

    if (myStepper.distanceToGo() == 0) {
      myStepper.moveTo(steps);
    }

    myStepper.run();

    if (myStepper.distanceToGo() == 0) {
      myStepper.disableOutputs();
    }

  } else {
    myStepper.disableOutputs();
  }
  currentAngle = myStepper.currentPosition() / stepsByDegree + 1;
}

void lcdTask() {
  static unsigned long lastTime = 0;
  if (currentTime - lastTime >= lcdDelay) {
    lastTime = currentTime;

    lcd.clear();
    lcd.print("Dist : ");
    lcd.print(distance);
    lcd.print(" cm");
    lcd.setCursor(0, 1);

    switch (state) {
      case alarm:
        lcd.print("ALERTE");
        break;
      case tooClose:
        lcd.print("Obj  : Trop Pres");
        break;
      case automatic:
        lcd.print("Angle: ");
        lcd.print(currentAngle);
        break;
      case tooFar:
        lcd.print("Obj  : Trop Loin");
        break;
    }
  }
}

// void serialTask() {
//   static unsigned long lastTime = 0;
//   if (currentTime - lastTime >= serialDelay) {
//     lastTime = currentTime;
//     Serial.print("etd:6307713,dist:");
//     Serial.print(distance);
//     Serial.print(",deg:");
//     Serial.println(currentAngle);
//   }
// }

void setColor(int red, int green, int blue) {
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}