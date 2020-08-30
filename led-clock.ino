/*
  LED clock using Arduino and one 60-LEDs stripe
  @version 1.0.1a
  Copyright (c) 2018 - P. Cheewahkidakarn (https://www.sovoboys.net/about/ihut/)
  
  @see README.md for more info
 */

#include <FastLED.h>
#include <DS3231.h>
#include <Bounce2.h>

#define LED_PIN             9
#define SWITCH_B_PIN        3
#define SWITCH_F_PIN        5
#define SWITCH_MODE_PIN     7
#define MIC_PIN             A0
//      SDA_PIN             A4  //  ** MUST BE FIXED, ONLY REF
//      SCL_PIN             A5  //  ** MUST BE FIXED, ONLY REF

// real leds, in case you want to test to > 60 strip
#define LEDS_NUM            60
// where is first LED in strip starts at in the clock?
#define SERIES_LED_START    30
// mine starts from the most bottom (6 o clock), so it starts at (number) 30

// (clap) sound tolarence, less means more sensitive
#define SOUND_TOLARENCE     45

// brightness values (as levels), some natural between 1 and 255
#define BRIGHTNESS_1        40
#define BRIGHTNESS_2        60
#define BRIGHTNESS_3        80
#define BRIGHTNESS_4        100
#define BRIGHTNESS_MAX      120

// modes
#define SWITCH_MODE_INIT    0
#define SWITCH_MODE_H       1
#define SWITCH_MODE_I       2
#define SWITCH_MODE_S       3
#define SWITCH_MODE_W       4
#define SWITCH_MODE_D       5
#define SWITCH_MODE_M       6
#define SWITCH_MODE_Y       7

#define CLAP_MODE_INIT      0
#define CLAP_MODE_CALENDAR  1
#define CLAP_MODE_TEMP      2

// (ms) time to display selected clap mode before back to CLAP_MODE_INIT
#define CLAP_TIMEDOUT                 8000
// (ms) time to debounce clap before next one
#define CLAP_DEBOUNCE_TIMEDOUT        600

CRGB    leds[LEDS_NUM];
DS3231  rtc(SDA, SCL);
Bounce  debouncerMode = Bounce();
Bounce  debouncerB = Bounce();
Bounce  debouncerF = Bounce();
Time    t;

unsigned char rotation;
unsigned int  switchMode =          SWITCH_MODE_INIT;
unsigned int  clapMode =            CLAP_MODE_INIT;
unsigned int  theFlash =            0;
unsigned int  blinkVal =            true;
int           latestSoundLevel =    -1;
int           currentSoundLevel =   0;
int           clapTimeout =         0;
int           clapDebounceTimeout = 10000; // fix bouncing at start

// > f(x)

// > f(x) - LEDs
// draw LED unit, from param (int) index between 0-59 to raw index starts at SERIES_LED_START
void drawUnit (int index, unsigned int red = 0, unsigned int green = 0, unsigned int blue = 0)
{
  // normalize rgb values (given params)
  red   = normalizeBrightnessValue(red);
  green = normalizeBrightnessValue(green);
  blue  = normalizeBrightnessValue(blue);
  index = (SERIES_LED_START + index) % 60;
  leds[index] = CRGB (red, green, blue);
}

void addUnitValue (int index, unsigned int red = 0, unsigned int green = 0, unsigned int blue = 0)
{
  // normalize rgb values (given params)
  index = (SERIES_LED_START + index) % 60;
  leds[index].r = normalizeBrightnessValue(leds[index].r + red);
  leds[index].g = normalizeBrightnessValue(leds[index].g + green);
  leds[index].b = normalizeBrightnessValue(leds[index].b + blue);
}

void clearUnit (int index)
{
  return drawUnit(index);
}

void clearAllUnits ()
{
  for (int x = 0; x < LEDS_NUM; x++)
  {
    clearUnit(x);
  }
}

unsigned int normalizeBrightnessValue (int val)
{
  return constrain(val, 0, BRIGHTNESS_MAX);
}
// < f(x) - LEDs

// > f(x) - rtc

Time optH (int h = 0)
{
  Time t = rtc.getTime();
  h = t.hour + h;
  while (h < 0) {h += 24;}
  rtc.setTime(h % 24, t.min, t.sec);
  return rtc.getTime();
}

Time optI (int i = 0)
{
  Time t = rtc.getTime();
  i = t.min + i;
  while (i < 0) {i += 60;}
  rtc.setTime(t.hour, i % 60, t.sec);
  return rtc.getTime();
}

Time optS (int s = 0)
{
  Time t = rtc.getTime();
  s = t.sec + s;
  while (s < 0) {s += 60;}
  rtc.setTime(t.hour, t.min, s % 60);
  return rtc.getTime();
}

Time optY (int y = 0)
{
  Time t = rtc.getTime();
  y = t.year + y;
  y < 2018 && (y = 2018);
  rtc.setDate(t.date, t.mon, y);
  return rtc.getTime();
}

Time optM (int m = 0)
{
  Time t = rtc.getTime();
  m = t.mon + m;
  while (m < 1) {m += 12;}
  rtc.setDate(t.date, m % 12, t.year);
  return rtc.getTime();
}

Time optD (int d = 0)
{
  int unsigned mod;
  Time t = rtc.getTime();
  d = t.date + d;
  while (d < 1) {d += 31;}
  if (t.mon == 4 || t.mon == 6 || t.mon == 9 || t.mon == 11) // 30 days months
    {
      mod = 30;
      d % 31 == 0 && (d = mod);
    }
    else if (t.mon == 2) // Feb
    {
      mod = t.year % 4 == 0 ? 29 : 28; // leap year?
      d % 31 == 0 && (d = mod);
    }
    else // 31 days months
    {
      mod = 31;
    }
  rtc.setDate(d % mod, t.mon, t.year);
  return rtc.getTime();
}

Time optW (int w = 0)
{
  Time t = rtc.getTime();
  w = (t.dow + w) % 7;
  rtc.setDOW(w == 0 ? 7 : w);
  return rtc.getTime();
}
// < f(x) - rtc

unsigned int toggleSwitchMode ()
{
  if      (switchMode == SWITCH_MODE_INIT)  {switchMode = SWITCH_MODE_H;}
  else if (switchMode == SWITCH_MODE_H)     {switchMode = SWITCH_MODE_I;}
  else if (switchMode == SWITCH_MODE_I)     {switchMode = SWITCH_MODE_S;}
  else if (switchMode == SWITCH_MODE_S)     {switchMode = SWITCH_MODE_W;}
  else if (switchMode == SWITCH_MODE_W)     {switchMode = SWITCH_MODE_D;}
  else if (switchMode == SWITCH_MODE_D)     {switchMode = SWITCH_MODE_M;}
  else if (switchMode == SWITCH_MODE_M)     {switchMode = SWITCH_MODE_Y;}
  else                                      {switchMode = SWITCH_MODE_INIT;}
  return switchMode;
}

unsigned int toggleClapMode ()
{
  unsigned int c = clapMode;
  if      (clapMode == CLAP_MODE_INIT)      {clapMode = CLAP_MODE_CALENDAR;}
  else if (clapMode == CLAP_MODE_CALENDAR)  {clapMode = CLAP_MODE_TEMP;}
  else                                      {clapMode = CLAP_MODE_INIT;}
  if (c != clapMode)
  {
    clapTimeout = CLAP_TIMEDOUT;
    clapDebounceTimeout = CLAP_DEBOUNCE_TIMEDOUT;
  }
  return clapMode;
}

unsigned int toggleBlinkSelectorVal()
{
  blinkVal = ! blinkVal;
  return blinkVal;
}

void countdownClapTimedout (unsigned int interval)
{
  clapTimeout > 0 && (clapTimeout = clapTimeout - interval);
  clapTimeout <= 0 && (clapMode = 0);
  clapDebounceTimeout > 0 && (clapDebounceTimeout = clapDebounceTimeout - interval);
}

void handleModeSwitch ()
{
  debouncerMode.update();
  debouncerMode.fell() && toggleSwitchMode();
}

void handleDirSwitches ()
{
  debouncerF.update();
  debouncerB.update();
  int v = debouncerF.fell() ? 1 : (debouncerB.fell() ? -1 : 0);
  if (v != 0)
  {
    if      (switchMode == SWITCH_MODE_H) {optH(v);}
    else if (switchMode == SWITCH_MODE_I) {optI(v);}
    else if (switchMode == SWITCH_MODE_S) {optS(v);}
    else if (switchMode == SWITCH_MODE_W) {optW(v);}
    else if (switchMode == SWITCH_MODE_D) {optD(v);}
    else if (switchMode == SWITCH_MODE_M) {optM(v);}
    else if (switchMode == SWITCH_MODE_Y) {optY(v);}
  }
}

void handleMic ()
{
  currentSoundLevel = analogRead(MIC_PIN);
  if (switchMode != SWITCH_MODE_INIT)
  {
    clapMode = CLAP_MODE_INIT; // always ignore claps if switch mode used
  }
  else if (switchMode == SWITCH_MODE_INIT && clapDebounceTimeout <= 0 && abs(currentSoundLevel - latestSoundLevel) >= SOUND_TOLARENCE)
  {
    toggleClapMode();
  }
  latestSoundLevel = currentSoundLevel;
}

void renderClock()
{
  clearAllUnits();
  t = rtc.getTime();
  int h = ((t.hour % 12) * 5) + floor(t.min / 12);
  addUnitValue(h,   BRIGHTNESS_4 * (switchMode == SWITCH_MODE_H && blinkVal ? 0 : 1), 0, 0);
  addUnitValue(h+1, BRIGHTNESS_1 * (switchMode == SWITCH_MODE_H && blinkVal ? 0 : 1), 0, 0);
  addUnitValue(h-1, BRIGHTNESS_1 * (switchMode == SWITCH_MODE_H && blinkVal ? 0 : 1), 0, 0);
  addUnitValue(t.min, 0, BRIGHTNESS_4 * (switchMode == SWITCH_MODE_I && blinkVal ? 0 : 1), 0);
  addUnitValue(t.sec, 0, 0, BRIGHTNESS_4 * (switchMode == SWITCH_MODE_S && blinkVal ? 0 : 1));
  addUnitValue(theFlash, BRIGHTNESS_1, BRIGHTNESS_1, BRIGHTNESS_1);
  addUnitValue(theFlash + 20, BRIGHTNESS_1, BRIGHTNESS_1, BRIGHTNESS_1);
  addUnitValue(theFlash + 40, BRIGHTNESS_1, BRIGHTNESS_1, BRIGHTNESS_1);
  theFlash = abs((theFlash + 1) % 60);
  FastLED.show();
}

void renderCalendar()
{
  clearAllUnits();
  t = rtc.getTime();
  addUnitValue((t.year % 2000) % 60, BRIGHTNESS_4 * (switchMode == SWITCH_MODE_Y && blinkVal ? 0 : 1), 0, 0);
  addUnitValue(t.mon * 5, 0, BRIGHTNESS_4 * (switchMode == SWITCH_MODE_M && blinkVal ? 0 : 1), 0);
  addUnitValue(t.date, 0, 0, BRIGHTNESS_4 * (switchMode == SWITCH_MODE_D && blinkVal ? 0 : 1));

  unsigned int dowVal = BRIGHTNESS_3 * (switchMode == SWITCH_MODE_W && blinkVal ? 0 : 1);
  for (unsigned int x = 51; x <= 54; x++)
  {
    if      (t.dow == SUNDAY)     {addUnitValue(x, dowVal, 0, 0);}
    else if (t.dow == MONDAY)     {addUnitValue(x, dowVal, dowVal, 0);}
    else if (t.dow == TUESDAY)    {addUnitValue(x, dowVal, 0, dowVal);}
    else if (t.dow == WEDNESDAY)  {addUnitValue(x, 0, dowVal, 0);}
    else if (t.dow == THURSDAY)   {addUnitValue(x, dowVal, abs(dowVal / 3), 0);}
    else if (t.dow == FRIDAY)     {addUnitValue(x, 0, abs(dowVal / 3), dowVal);}
    else if (t.dow == SATURDAY)   {addUnitValue(x, dowVal, 0, abs(dowVal / 3));}
  }
  FastLED.show();
}

void renderTemp()
{
  clearAllUnits();
  int tempC = round(rtc.getTemp());
  tempC < 0 && (tempC = 0);
  addUnitValue(tempC, BRIGHTNESS_4, BRIGHTNESS_4, 0);
  FastLED.show();
}

// < f(x)

void runBabyRun()
{
  if (switchMode != SWITCH_MODE_INIT) // priority - switch mode first
  {
    clapMode = CLAP_MODE_INIT;
    if (switchMode == SWITCH_MODE_W || switchMode == SWITCH_MODE_D || switchMode == SWITCH_MODE_M || switchMode == SWITCH_MODE_Y)
    {
      renderCalendar();
    }
    else
    {
      renderClock();
    }
  }
  else if (clapMode != CLAP_MODE_INIT) // priority - clap mode next
  {
    clapMode == CLAP_MODE_CALENDAR ? renderCalendar() : renderTemp();
  }
  else
  {
    renderClock();
  }
}

void setup()
{
  rtc.begin();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LEDS_NUM);
  FastLED.setBrightness(20);
  debouncerMode.attach(SWITCH_MODE_PIN, INPUT_PULLUP);
  debouncerB.attach(SWITCH_B_PIN, INPUT_PULLUP);
  debouncerF.attach(SWITCH_F_PIN, INPUT_PULLUP);
  debouncerMode.interval(20);
  debouncerB.interval(20);
  debouncerF.interval(20);
}

void loop()
{
  EVERY_N_MILLISECONDS (30)
  {
    runBabyRun();
    countdownClapTimedout(30); // as same as interval @ EVERY_N_MILLISECONDS parent block
  }

  EVERY_N_MILLISECONDS (300)
  {
    toggleBlinkSelectorVal();
  }
  handleModeSwitch();
  handleDirSwitches();
  handleMic();
}
