#include <LCD_I2C.h>
#include <AccelStepper.h>
#include <HCSR04.h>
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


LCD_I2C lcd(0x27, 16, 2);

AccelStepper myStepper(AccelStepper::FULL4WIRE, IN_1, IN_3, IN_2, IN_4);

enum State { alarm,
             tooClose,
             automatic,
             tooFar };

State state;

unsigned long currentTime;
unsigned long lcdLastTime = 0;
unsigned long serialLastTime = 0;
unsigned long lastAlarmTriggerTime = 0;

float stepsByDegree = 2038.0 / 360.0;
int angle = 0;
int currentAngle = 0;
int steps = 0;

int distance = 0;
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

  getDistance();

  stateManager();

  runAlarm();

  runMotor();

  lcdTask();

  serialTask();
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
    analogWrite(BUZZER_PIN, 2);
    if (currentTime - lastTime >= alarmBlinkDelay) {
      lastTime = currentTime;
      ledState = !ledState;
    }
    if (ledState) {
      setColor(0,0,255);
    } else {
      setColor(255,0,0);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    setColor(0,0,0);
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

void serialTask() {
  static unsigned long lastTime = 0;
  if (currentTime - lastTime >= serialDelay) {
    lastTime = currentTime;
    Serial.print("etd:6307713,dist:");
    Serial.print(distance);
    Serial.print(",deg:");
    Serial.println(currentAngle);
  }
}

void setColor(int red, int green, int blue) {
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}