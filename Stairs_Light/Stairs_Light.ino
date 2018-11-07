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

#define DISP_CLK 5
#define DISP_DIO 4

#define UP_SONIC_TRIG_PIN 9
#define UP_SONIC_ECHO_PIN 10

#define DW_SONIC_TRIG_PIN 11
#define DW_SONIC_ECHO_PIN 12

#define ECODE_CNT_CLK_PIN 1 //ITEM_COUNT
#define ECODE_CNT_DT_PIN 0
boolean DT_CNT_now;
boolean DT_CNT_last;
long counter_CNT = 10;

#define ECODE_LS_CLK_PIN 3 //LIGHT_SPEED (miliseconds per step)
#define ECODE_LS_DT_PIN 2
boolean DT_LS_now;
boolean DT_LS_last;
long counter_LS = 500;

#define ECODE_MB_CLK_PIN 8 //MAX_BRIGHT;
#define ECODE_MB_DT_PIN 7
boolean DT_MB_now;
boolean DT_MB_last;
long counter_MB = 255;//0-255

#define ECODE_WB_CLK_PIN 17 //WAIT_BRIGHT;
#define ECODE_WB_DT_PIN 16
boolean DT_WB_now;
boolean DT_WB_last;
long counter_WB = 1;//0-255

long settingDisplayData;

#define LED_DATA_PIN 6

//modes
#define WAITING_MODE 0
#define TEST_MODE 1
#define GO_MODE 2
#define SETTING_MODE 3
#define SLEEP_MODE 4

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

bool isSettingActive = false;
bool isDLTchanged = false;
bool isLSchanged = false;
bool isMBchanged = false;
bool isWBchanged = false;

bool isGoUp = false;
bool isGoDw = false;

int softLightPersent = 30;

void printOnDisplay(String text, int number = -1)
{
  display.clearDisplay();

  display.drawPixel(0, 0, WHITE);
  display.drawPixel(127, 0, WHITE);
  display.drawPixel(0, 31, WHITE);
  display.drawPixel(127, 31, WHITE);
//
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(27, 5);
//  display.print(text);
//  if (number != -1)
//  {
//    display.setCursor(27, 15);
//    display.print(number);
//  }

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
  //disp.init();
  //  disp.set(7);  //bright, 0 - 7 (min - max)
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
  //ECODE
  pinMode (ECODE_CNT_CLK_PIN, INPUT);
  pinMode (ECODE_CNT_DT_PIN, INPUT);
  DT_CNT_last = digitalRead(ECODE_CNT_CLK_PIN);
  pinMode (ECODE_LS_CLK_PIN, INPUT);
  pinMode (ECODE_LS_DT_PIN, INPUT);
  DT_LS_last = digitalRead(ECODE_LS_CLK_PIN);
  pinMode (ECODE_MB_CLK_PIN, INPUT);
  pinMode (ECODE_MB_DT_PIN, INPUT);
  DT_MB_last = digitalRead(ECODE_MB_CLK_PIN);
  pinMode (ECODE_WB_CLK_PIN, INPUT);
  pinMode (ECODE_WB_DT_PIN, INPUT);
  DT_WB_last = digitalRead(ECODE_WB_CLK_PIN);
}

void loop()
{
  bool isTESTpressed = (digitalRead(TEST_BTN_PIN) == HIGH);
  bool isMODEpressed = (digitalRead(MODE_BTN_PIN) == HIGH);
  bool isPLUSpressed = (digitalRead(PLUS_BTN_PIN) == HIGH);
  bool isMINUSpressed = (digitalRead(MINUS_BTN_PIN) == HIGH);
  printOnDisplay("Hello, world!", isMODEpressed);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  if (isTESTpressed)
  {
    display.setCursor(5, 5);
    display.print("T");
  }

  if (isMODEpressed)
  {
    display.setCursor(15, 5);
    display.print("M");
  }

  if (isPLUSpressed)
  {
    display.setCursor(25, 5);
    display.print("+");
  }
  if (isMINUSpressed)
  {
    display.setCursor(35, 5);
    display.print("-");
  }
  display.display();


  return;
  //###################### MODE DETERMINATION
  //TEST MODE DETECT
  if (digitalRead(TEST_BTN_PIN) == HIGH)
  {
    changeModeTo(TEST_MODE);
  }
  else
  {
    //SETTING MODE DETECT
    isDLTchanged = encoderTick(ECODE_CNT_DT_PIN, ECODE_CNT_CLK_PIN, DT_CNT_now, DT_CNT_last, counter_CNT);
    isLSchanged = encoderTick(ECODE_LS_DT_PIN, ECODE_LS_CLK_PIN, DT_LS_now, DT_LS_last, counter_LS);
    isMBchanged = encoderTick(ECODE_MB_DT_PIN, ECODE_MB_CLK_PIN, DT_MB_now, DT_MB_last, counter_MB);
    isWBchanged = encoderTick(ECODE_WB_DT_PIN, ECODE_WB_CLK_PIN, DT_WB_now, DT_WB_last, counter_WB);
    if (false)//(isDLTchanged || isLSchanged || isMBchanged || isWBchanged)
    {
      changeModeTo(SETTING_MODE);
      valideteSetting();
      if (isDLTchanged) settingDisplayData = counter_CNT;
      if (isLSchanged) settingDisplayData = counter_LS;
      if (isMBchanged) settingDisplayData = counter_MB;
      if (isWBchanged) settingDisplayData = counter_WB;
    }
    if ( MODE != SETTING_MODE)
    {
      if (isThereDayLight())
      {
        changeModeTo(SLEEP_MODE);
      }
      else
      {
        updateSonicDataUp();
        updateSonicDataDw();
        if (!isGoUp)
        {
          isGoUp = UP_SONIC_DIST <= UP_S_TRIGGER;
        }
        if (!isGoDw)
        {
          isGoDw = DW_SONIC_DIST <= DW_S_TRIGGER;
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

  if (MODE == SETTING_MODE)
  {
    timerSett += delayTime;
    if (timerSett > settinDelay)
    {
      timerSett = 0;
      changeModeTo(WAITING_MODE);
    }
  }
  else
  {
    if (isGoUp)
      timerUp += delayTime;
    if (isGoDw)
      timerDw += delayTime;
    const long fullTime = counter_CNT * counter_LS;

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
}

bool valideteSetting()
{
  if (counter_CNT < 2)
    counter_CNT = 2;
  if (counter_CNT > MAX_ITEM_COUNT)
    counter_CNT = MAX_ITEM_COUNT;

  if (counter_LS < 100)
    counter_LS = 100;
  if (counter_LS > 10000)
    counter_LS = 10000;

  if (counter_MB < 1)
    counter_MB = 1;
  if (counter_MB > 255)
    counter_MB = 255;

  if (counter_WB < 1)
    counter_WB = 1;
  if (counter_WB > 255)
    counter_WB = 255;
}

bool isThereDayLight()
{
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

  //  Serial.print("Distance up: ");
  //  Serial.println(UP_SONIC_DIST );
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

  //  Serial.print("Distance: ");
  //  Serial.print(DW_SONIC_DIST );
}

void displayData()
{
  switch (MODE)
  {
    case WAITING_MODE:
      printOnDisplay("Wait for move");
      // disp.displayByte(_U, _A, _l, _t);
      brightWait();
      break;
    case TEST_MODE:
      printOnDisplay("Test mode");
      //disp.displayByte(_t, _E, _S, _t);
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
    case SETTING_MODE:
      printOnDisplay("Setting");
      // disp.displayIntZero(settingDisplayData);//mode
      break;
    case SLEEP_MODE:
      printOnDisplay("Sleep");
      // disp.displayByte(_S, _L, _E, _P);
      shutDownAllPins();
      FastLED.show();
      break;
  }
}

void brightAll()
{
  shutDownAllPins();
  for (int i = 0; i < counter_CNT; ++i)
    leds[i] = CRGB(counter_MB, counter_MB, counter_MB);
  FastLED.show();
}

void brightWait()
{
  shutDownAllPins();
  leds[0] =             CRGB(counter_WB, counter_WB, counter_WB);
  leds[counter_CNT - 1] = CRGB(counter_WB, counter_WB, counter_WB);
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
    int _stepUp = timerUp / (counter_LS * index);
    if (_stepUp < counter_CNT)
      leds[_stepUp] = softLight(_stepUp * counter_LS * index, (counter_LS * index), timerUp, counter_MB);
  }
  if (isGoDw)
  {
    int _stepDw = counter_CNT - 1 - timerDw / (counter_LS * index);
    if (_stepDw < counter_CNT)
      leds[_stepDw] = softLight(timerDw / (counter_LS * index) * counter_LS * index, (counter_LS * index), timerDw, counter_MB);
  }
  FastLED.show();
}

void shutDownAllPins()
{
  for (int i = 0; i < MAX_ITEM_COUNT; ++i)
    leds[i] = CRGB(0, 0, 0);
  //need to show() for apply
}

bool encoderTick(const int DT, const int CLK, boolean & DT_now, boolean & DT_last, long & counter)
{
  DT_now = digitalRead(CLK);
  if (DT_now != DT_last)
  {
    if (digitalRead(DT) != DT_now)
    {
      counter ++;
    }
    else
    {
      counter --;
    }
  }
  bool res = (DT_last != DT_now);
  DT_last = DT_now;
  return res;
}
