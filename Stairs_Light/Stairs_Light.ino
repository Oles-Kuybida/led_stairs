#include "FastLED.h"
#include <Wire.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <NewPing.h>

//pins
#define rxPin 2 // Wire this to Tx Pin of ESP8266
#define txPin 3 // Wire this to Rx Pin of ESP8266

//day light sensor
#define LIGHT_REGISTOR_PIN 5
boolean IS_DAY_LIGHT;

#define LED_DATA_PIN 6

//sonic_0
#define SONIC_TRIG_PIN 7
#define SONIC_ECHO_PIN 8
#define MAX_DISTANCE 500
int UP_S_TRIGGER = 2; //cm    //up sonic trigger dist
NewPing sonar(SONIC_TRIG_PIN, SONIC_ECHO_PIN, MAX_DISTANCE);
int SONIC_DIST;

//sonic_1 or ir
//#define SONIC_TRIG_PIN 9
//#define SONIC_ECHO_PIN 10
#define IR_SENSOR_PIN 9
boolean IS_IR_MOVE;

#define delayTime 20
#define settinDelay 2000

int ITEM_COUNT = 15; //stair count
int ON_ITEM_COUNT = 15;

long STEP_DURATION = 500;//(miliseconds per step)
int MAX_BRIGHT = 255;//0-255
int WAIT_BRIGHT = 1;//0-255

//modes
#define TEST_MODE 10
#define GO_MODE 20

#define WAITING_MODE 0
#define SETTING_ITEM_COUNT 1
#define SETTING_STEP_DURATION 2
#define SETTING_MAX_BRIGHT 3
#define SETTING_WAIT_BRIGHT 4
#define SLEEP_MODE 5

int test_mode_value;//-1==OFF
#define ALL_TEST = 0;
#define UP_TEST = 1;
#define DOWN_TEST = 2;

//global variables
int MODE;
long timerUp = 0;
long timerDw = 0;
long timerSett = 0;

#define MAX_ITEM_COUNT 346
CRGB leds[MAX_ITEM_COUNT ];//max count
int stair[MAX_ITEM_COUNT];

bool isGoUp = false;
bool isGoDw = false;

int softLightPersent = 30;

String errorText = "no error";
int errorCode = 0;

SoftwareSerial ESP8266 (rxPin, txPin);

typedef struct record Stair;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
String sendATcomand(String cmd, int delayT = 200, boolean debug = false)
{
  String response = "";
  response = "NO RESPONSE";
  ESP8266.println(cmd);
  Serial.println(cmd);

  delay(delayT);
  while (ESP8266.available()) {
    response = ESP8266.readStringUntil('\n');
  }
  if (debug)
  {
    String finalStr = "Got reponse from ESP8266: " + response;
    Serial.println(finalStr);
  }
  return response;
}

void initESP()
{
  ESP8266.begin(115200);
  ESP8266.println("AT+RST");

  delay(1000);

  if (ESP8266.find("OK") ) Serial.println("Module Reset");

  Serial.println("ESP READY");

  // set the data rate for the SoftwareSerial port
  sendATcomand("AT+RST", 2000, true);  //reset
  //sendATcomand("AT", 2000, true);  //test

  sendATcomand("AT+CWMODE=3", 1000, true); //mode station
  //sendATcomand("AT+CWLAP", 4000, true);
  sendATcomand("AT+CWJAP=\"Owl_x_2G\",\"owl{}123\"", 2000, true); //connect to Wi Fi AP
  sendATcomand("AT+CWMODE=1", 1000, true); //mode station
  sendATcomand("AT+CIPMUX=1", 1000, true); //mode station
  sendATcomand("AT+CIFSR", 5000, true); // get ip address
  sendATcomand("AT+CIPSERVER=1,80", 1000, true); // turn on server on port 80
}

void updateSensorDataSonicOrIrUp()
{
  //Sonic
  SONIC_DIST = sonar.ping_cm();
  //  Serial.print("Distance up: ");
  //  Serial.print(SONIC_DIST );

  if (SONIC_DIST != 0)
  {
    errorText = "no error";
    errorCode = 0;
  }
}

void updateSensorDataSonicOrIrDw()
{
  IS_IR_MOVE = LOW;//digitalRead(IR_SENSOR_PIN);
  //  Serial.print("Ir sensor up: ");
  //  Serial.print(IS_IR_MOVE );
}

void setup()
{
  //  stair[0] = 24;
  //  stair[1] = 23;
  //  stair[2] = 23;
  //  stair[3] = 23;
  //  stair[4] = 23;
  //  stair[5] = 23;
  //  stair[6] = 23;
  //  stair[7] = 23;
  //  stair[8] = 23;
  //  stair[9] = 23;
  //  stair[10] = 23;
  //  stair[11] = 23;
  //  stair[12] = 23;
  //  stair[13] = 23;
  //  stair[14] = 23;
  //  stair[15] = 23;
  stair[0] = 2;
  stair[1] = 1;
  stair[2] = 1;
  stair[3] = 1;
  stair[4] = 1;
  stair[5] = 1;
  stair[6] = 1;
  stair[7] = 1;
  stair[8] = 1;
  stair[9] = 1;
  stair[10] = 1;
  stair[11] = 1;
  stair[12] = 1;
  stair[13] = 1;
  stair[14] = 1;
  stair[15] = 2;

  Serial.println("Setup");
  Serial.begin(9600);

  Serial.println("esp");
  //initESP();
  Serial.println("esp ready");
  //led
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, MAX_ITEM_COUNT );

  //
  pinMode(IR_SENSOR_PIN , INPUT);
  //sonic
  pinMode(SONIC_TRIG_PIN , OUTPUT);
  pinMode(SONIC_ECHO_PIN , INPUT);

  pinMode(LIGHT_REGISTOR_PIN, INPUT);

  Serial.println("Setup ready");
  //ReadSetting();
}

void loop()
{
  errorText = "no init";
  errorCode = 0;
  if (isThereDayLight())
  {
    changeModeTo(SLEEP_MODE);
  }
  else
  {
    if (MODE == SLEEP_MODE)
    {
      changeModeTo(WAITING_MODE);
      isGoUp =  false;
      isGoDw =  false;
    }
  }
  bool isTESTpressed = false;
  bool isMODEpressed = false;
  //###################### MODE DETERMINATION
  {
    if ( (MODE == WAITING_MODE) || (MODE == GO_MODE) )
    {
      if (!isGoUp)
        updateSensorDataSonicOrIrUp();

      if (!isGoDw)
        updateSensorDataSonicOrIrDw();

      if (SONIC_DIST == 0)
      {
        errorText = "NoUpSonic";
        errorCode = 1;
        isGoUp = false;
      }
      if (errorCode == 0)
      {
        if (!isGoUp)
        {
          isGoUp = SONIC_DIST <= UP_S_TRIGGER;
        }
        if (!isGoDw)
        {
          isGoDw = (IS_IR_MOVE == HIGH);
        }
      }
      if (MODE != GO_MODE)
      {
        if ( isGoUp || isGoDw )
        {
          changeModeTo(GO_MODE);
        }
        else
          changeModeTo(WAITING_MODE);
      }
    }
  }
  //#################################################
  //processing mode
  displayData();

  //*************************************************
  delay(delayTime);

  if (isGoUp)
    timerUp += delayTime;
  if (isGoDw)
    timerDw += delayTime;
  const long fullTime = (ITEM_COUNT * STEP_DURATION) + (ON_ITEM_COUNT * STEP_DURATION);

  if ((timerUp >= fullTime) || !isGoUp)
  {
    isGoUp = false;
    timerUp = 0;
  }
  if ((timerDw >= fullTime) || !isGoDw)
  {
    isGoDw = false;
    timerDw = 0;
  }
  if (MODE == GO_MODE && !isGoDw && !isGoUp)
    changeModeTo(WAITING_MODE);
}

bool valideteSetting()
{
  if (ITEM_COUNT < 2)
    ITEM_COUNT = 2;
  if (ITEM_COUNT > MAX_ITEM_COUNT)
    ITEM_COUNT = MAX_ITEM_COUNT;

  if (STEP_DURATION < 100)
    STEP_DURATION = 100;
  if (STEP_DURATION > 10000)
    STEP_DURATION = 10000;

  if (MAX_BRIGHT < 1)
    MAX_BRIGHT = 1;
  if (MAX_BRIGHT > 255)
    MAX_BRIGHT = 255;

  if (WAIT_BRIGHT < 1)
    WAIT_BRIGHT = 1;
  if (WAIT_BRIGHT > 255)
    WAIT_BRIGHT = 255;

  // SaveSetting();
}

boolean isThereDayLight()
{
  //check light sensor if is day or night now
  int dayLight = digitalRead(LIGHT_REGISTOR_PIN);
  Serial.print("");
  IS_DAY_LIGHT = dayLight;
  if (dayLight)
    return false;
  return true;
}

void changeModeTo(int mode)
{
  timerUp = 0;
  timerDw = 0;
  MODE = mode;
  Serial.print("Mode changed: ");
  Serial.println(MODE);
}

void displayData()
{
  if (errorCode != 0)
  {
    Serial.print("Error ");
    Serial.print(errorCode);
    Serial.println(errorText);
    shutDownAllPins();
    return;
  }
  switch (MODE)
  {
    case WAITING_MODE:
      brightWait();
      break;
    case TEST_MODE:
      brightAll();
      break;
    case GO_MODE:
      {
        bright();
      }
      break;
    case SETTING_ITEM_COUNT:
      break;
    case SETTING_STEP_DURATION:
      break;
    case SETTING_MAX_BRIGHT:
      break;
    case SETTING_WAIT_BRIGHT:
      break;
    case SLEEP_MODE:
      shutDownAllPins();
      FastLED.show();
      break;
  }
}

void brightAll()
{
  shutDownAllPins();
  for (int i = 0; i < ITEM_COUNT; ++i)
    stairOn(i, CRGB(MAX_BRIGHT, MAX_BRIGHT, MAX_BRIGHT));
  FastLED.show();
}

void brightWait()
{
  shutDownAllPins();
  stairOn(0, CRGB(WAIT_BRIGHT, WAIT_BRIGHT, WAIT_BRIGHT));
  stairOn(ITEM_COUNT - 1, CRGB(WAIT_BRIGHT, WAIT_BRIGHT, WAIT_BRIGHT));
  FastLED.show();
}

CRGB softLight(long startT, long duration, long currT, int MAX, bool on = true,  bool off = true)
{
  int softTime = duration * softLightPersent / 100;
  if ( ((currT - startT) < softTime) && on)
  {
    int level = ((currT - startT) * MAX / softTime);
    return CRGB(level, level, level);
  }
  if ( (currT > (startT + duration - softTime)) && off)
  {
    int level = ((startT + duration - currT) * MAX / softTime);
    return CRGB(level, level, level);
  }
  return CRGB(MAX, MAX, MAX);
}

void bright()
{
  int index = 1;//time setting const
  CRGB colrmax = CRGB(MAX_BRIGHT, MAX_BRIGHT, MAX_BRIGHT);
  shutDownAllPins();
  if (isGoUp)
  {
    int _stepUp = timerUp / (STEP_DURATION * index);
    if (_stepUp < ITEM_COUNT + ON_ITEM_COUNT)
    {
      CRGB c = softLight(_stepUp * STEP_DURATION * index, (STEP_DURATION * index), timerUp, MAX_BRIGHT);
      for (int j = 0; j < ON_ITEM_COUNT; ++j)
      {
        int currentStep = _stepUp - j;
        if (currentStep >= ITEM_COUNT)
          continue;
        if (j == 0)
          stairOn(currentStep, c);
        else
          stairOn(currentStep, colrmax);
      }
      //      Serial.print("stair up ");
      //      Serial.println(_stepUp);
    }
  }
  if (isGoDw)
  {
    int _stepDw = ITEM_COUNT - 1 - timerDw / (STEP_DURATION * index);
    if (_stepDw < ITEM_COUNT)
    {
      CRGB c = softLight(timerDw / (STEP_DURATION * index) * STEP_DURATION * index, (STEP_DURATION * index), timerDw, MAX_BRIGHT);
      stairOn(_stepDw, c);
      //      Serial.print("stair dw ");
      //      Serial.println(_stepDw);
    }
  }
  FastLED.show();
}

void shutDownAllPins()
{
  for (int i = 0; i < MAX_ITEM_COUNT; ++i)
    leds[i] = CRGB(0, 0, 0);
  //need to show() for apply
}

void SaveSetting()
{
  EEPROM.write(10, ITEM_COUNT);
  EEPROM.write(20, STEP_DURATION);
  EEPROM.write(30, MAX_BRIGHT);
  EEPROM.write(40, WAIT_BRIGHT);
}

void ReadSetting()
{
  ITEM_COUNT = EEPROM.read(10);
  STEP_DURATION = EEPROM.read(20);
  MAX_BRIGHT = EEPROM.read(30);
  WAIT_BRIGHT = EEPROM.read(40);
}

void stairOn (int index, CRGB color)
{
  int firstSindex = 0;
  int lastSindex = stair[0] - 1;
  for (int i = 0; i < index; ++i)
  {
    firstSindex += stair[i];
    lastSindex = firstSindex + stair[i] - 1;
  }
  for (int j = firstSindex; j <= lastSindex; ++j)
    leds[j] = color;
}
