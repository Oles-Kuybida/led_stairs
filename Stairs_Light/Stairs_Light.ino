#include "FastLED.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define delayTime 20
#define settinDelay 2000
//pins
#define TEST_BTN_PIN 4
#define MODE_BTN_PIN 5
#define PLUS_BTN_PIN 2
#define MINUS_BTN_PIN 3

#define LIGHT_REGISTOR_PIN A1

#define UP_SONIC_TRIG_PIN 9
#define UP_SONIC_ECHO_PIN 10

#define DW_SONIC_TRIG_PIN 11
#define DW_SONIC_ECHO_PIN 12

int ITEM_COUNT = 10;
long STEP_DURATION = 500;//(miliseconds per step)
int MAX_BRIGHT = 255;//0-255
int WAIT_BRIGHT = 1;//0-255

#define LED_DATA_PIN 8

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

int UP_SONIC_DIST;    //up
int DW_SONIC_DIST;   //down

//settings
int UP_S_TRIGGER = 10; //cm    //up sonic trigger dist
int DW_S_TRIGGER = 10; //cm   //down  sonic trigger dist

#define MAX_ITEM_COUNT 50
CRGB leds[MAX_ITEM_COUNT ];//max count

// OLED display TWI address
#define OLED_ADDR   0x3C

// reset pin not used on 4-pin OLED module
Adafruit_SSD1306 display(-1);  // -1 = no reset pin

bool isGoUp = false;
bool isGoDw = false;

int softLightPersent = 30;

String errorText = "no error";
int errorCode = 0;

void printOnDisplay(String text, int number = -1)
{
  display.clearDisplay();

  display.drawPixel(0, 0, WHITE);
  display.drawPixel(127, 0, WHITE);
  display.drawPixel(0, 31, WHITE);
  display.drawPixel(127, 31, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(27, 5);
  display.print(text);
  if (number != -1)
  {
    display.setCursor(27, 15);
    display.print(number);
  }

  display.display();
}

void setup()
{
  Serial.begin(9600);
  //led
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, MAX_ITEM_COUNT );
  //disp
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  //sonic
  pinMode(UP_SONIC_TRIG_PIN , OUTPUT);
  pinMode(UP_SONIC_ECHO_PIN , INPUT);
  pinMode(DW_SONIC_TRIG_PIN , OUTPUT);
  pinMode(DW_SONIC_ECHO_PIN , INPUT);
  //Buttons
  pinMode(TEST_BTN_PIN, INPUT);
  pinMode(MODE_BTN_PIN, INPUT);
  pinMode(PLUS_BTN_PIN, INPUT);
  pinMode(MINUS_BTN_PIN, INPUT);
}

void loop()
{
  bool isTESTpressed = (digitalRead(TEST_BTN_PIN) == HIGH);
  bool isMODEpressed = (digitalRead(MODE_BTN_PIN) == HIGH);
  bool isPLUSpressed = (digitalRead(PLUS_BTN_PIN) == HIGH);
  bool isMINUSpressed = (digitalRead(MINUS_BTN_PIN) == HIGH);
  //printOnDisplay("Hello, world!", isMODEpressed);

  //  display.setTextSize(1);
  //  display.setTextColor(WHITE);
  //  if (isTESTpressed)
  //  {
  //    display.setCursor(5, 5);
  //    display.print("T");
  //  }
  //
  //  if (isMODEpressed)
  //  {
  //    display.setCursor(15, 5);
  //    display.print("M");
  //  }
  //
  //  if (isPLUSpressed)
  //  {
  //    display.setCursor(25, 5);
  //    display.print("+");
  //  }
  //  if (isMINUSpressed)
  //  {
  //    display.setCursor(35, 5);
  //    display.print("-");
  //  }
  //  display.display();

  //###################### MODE DETERMINATION
  //TEST MODE DETECT
  if (isTESTpressed)
  {
    changeModeTo(TEST_MODE);
  }
  else
  {
    //SETTING MODE DETECT
    if (isMODEpressed)
    {
      int newMode;
      if ((MODE < 0) || (MODE > 5))
        newMode = 0;
      else
        newMode = MODE + 1;
      if (newMode > 5)
        newMode = 0;

      changeModeTo(newMode);
      //valideteSetting();
    }
    if ( (MODE == WAITING_MODE) || (MODE == GO_MODE) )
    {
      if (isThereDayLight())
      {
        changeModeTo(SLEEP_MODE);
      }
      else
      {
        updateSonicDataUp();
        updateSonicDataDw();
        UP_SONIC_DIST = 1; //TODO:
        DW_SONIC_DIST = 45; //TODO:
        //if no sonic
        if (UP_SONIC_DIST == 0)
        {
          errorCode = 1;
          errorText = "NoUpSonic";
        }
        if (DW_SONIC_DIST == 0)
        {
          errorCode = 2;
          errorText = "NoDownSonic";
        }
        if (errorCode == 0)
        {
          if (!isGoUp)
          {
            isGoUp = UP_SONIC_DIST <= UP_S_TRIGGER;
          }
          if (!isGoDw)
          {
            isGoDw = DW_SONIC_DIST <= DW_S_TRIGGER;
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
  const long fullTime = ITEM_COUNT * STEP_DURATION;

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
}

bool isThereDayLight()
{
  return false;
  //check light sensor if is day or night now
  Serial.println(analogRead(LIGHT_REGISTOR_PIN));
  if (analogRead(LIGHT_REGISTOR_PIN) < 500)
    return false;
  return true;//for debug/ must be true
}

void changeModeTo(int mode)
{
  timerUp = 0;
  timerDw = 0;
  MODE = mode;
  Serial.print("Mode changed: ");
  Serial.println(MODE);
}

void updateSonicDataUp()
{
  //update Up sonic data
  long duration;
  digitalWrite(UP_SONIC_TRIG_PIN , LOW);
  delayMicroseconds(2);
  digitalWrite(UP_SONIC_TRIG_PIN , HIGH);
  delayMicroseconds(10);
  digitalWrite(UP_SONIC_TRIG_PIN , LOW);

  duration = pulseIn(UP_SONIC_ECHO_PIN , HIGH);
  UP_SONIC_DIST = (duration * .0343) / 2;

  Serial.print("Distance up: ");
  Serial.println(UP_SONIC_DIST );
}

void updateSonicDataDw()
{
  //update Down sonic data
  long duration;
  digitalWrite(DW_SONIC_TRIG_PIN , LOW);
  delayMicroseconds(2);
  digitalWrite(DW_SONIC_TRIG_PIN , HIGH);
  delayMicroseconds(10);
  digitalWrite(DW_SONIC_TRIG_PIN , LOW);

  duration = pulseIn(DW_SONIC_ECHO_PIN , HIGH);
  DW_SONIC_DIST = (duration * .0343) / 2; //meter

  Serial.print("Distance dw: ");
  Serial.print(DW_SONIC_DIST );
}

void displayData()
{
  if (errorCode != 0)
  {
    printOnDisplay(errorText);
    return;
  }
  switch (MODE)
  {
    case WAITING_MODE:
      printOnDisplay("Wait for move");
      brightWait();
      break;
    case TEST_MODE:
      printOnDisplay("Test mode");
      brightAll();
      break;
    case GO_MODE:
      {
        int sec = 0;
        if (isGoUp)
          sec = timerUp / 1000;
        else
          sec = timerDw / 1000;
        int sec0 = sec % 10;
        int sec1 = (sec - sec0) % 100;
        printOnDisplay("GO!!", sec);
        //disp.displayByte(0, _G);
        // disp.displayByte(1, _O);
        // disp.display(2, sec1);//timer(reg0)
        // disp.display(3, sec0);//timer(reg1)
        bright();
      }
      break;
    case SETTING_ITEM_COUNT:
      printOnDisplay("Setting ITEM COUNT");
      break;
    case SETTING_STEP_DURATION:
      printOnDisplay("Setting STEP DURATION");
      break;
    case SETTING_MAX_BRIGHT:
      printOnDisplay("Setting MAX BRIGHT");
      break;
    case SETTING_WAIT_BRIGHT:
      printOnDisplay("Setting WAIT BRIGHT");
      break;
    case SLEEP_MODE:
      printOnDisplay("Sleep");
      shutDownAllPins();
      FastLED.show();
      break;
  }
}

void brightAll()
{
  shutDownAllPins();
  for (int i = 0; i < ITEM_COUNT; ++i)
    leds[i] = CRGB(MAX_BRIGHT, MAX_BRIGHT, MAX_BRIGHT);
  FastLED.show();
}

void brightWait()
{
  shutDownAllPins();
  leds[0] =             CRGB(WAIT_BRIGHT, WAIT_BRIGHT, WAIT_BRIGHT);
  leds[ITEM_COUNT - 1] = CRGB(WAIT_BRIGHT, WAIT_BRIGHT, WAIT_BRIGHT);
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
  shutDownAllPins();
  if (isGoUp)
  {
    int _stepUp = timerUp / (STEP_DURATION * index);
    if (_stepUp < ITEM_COUNT)
      leds[_stepUp] = softLight(_stepUp * STEP_DURATION * index, (STEP_DURATION * index), timerUp, MAX_BRIGHT);
  }
  if (isGoDw)
  {
    int _stepDw = ITEM_COUNT - 1 - timerDw / (STEP_DURATION * index);
    if (_stepDw < ITEM_COUNT)
      leds[_stepDw] = softLight(timerDw / (STEP_DURATION * index) * STEP_DURATION * index, (STEP_DURATION * index), timerDw, MAX_BRIGHT);
  }
  FastLED.show();
}

void shutDownAllPins()
{
  for (int i = 0; i < MAX_ITEM_COUNT; ++i)
    leds[i] = CRGB(0, 0, 0);
  //need to show() for apply
}
