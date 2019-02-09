//We always have to include the library
#include <LedControl.h>
#include <Wire.h>
#include <RTClib.h>
#include <Bounce2.h>

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
#define NUMBER_OF_DIGITS 4
#define TEMP_CHAR 'F'
#define TEMP_TIMEOUT 15;
#define BUTTON_SET_PIN 2 //
#define BUTTON_FUNCTION_PIN 3
#define BUTTON_TEMP_BUTTON 4  //WILL BE USED FOR  DECREASE
#define BUTTON_TIMER_BUTTON 5 //WILL BE USED FOR INCREASE
#define DEBOUNCE 25

char tempChar = TEMP_CHAR;

Bounce setButton = Bounce();
Bounce functionButton = Bounce();
//ALT USE DECREASE
Bounce tempButton = Bounce();
// ALT USE INCREASE
Bounce timerButton = Bounce();

//SOFTWARE

enum DisplayState
{
  TIME,
  TEMP,
  CONFIG,
  TIMER,
};
// States of the timer
enum TimerState
{
  // entry state of the timer.
  STOPPED,
  // pre exit state of the timer.
  PAUSED,
  // run state of the timer.
  RUNNING,
};

// States of configuration to go through
enum SetState
{
  SECOND,
  MINUTE,
  HOUR,
  DAY,
  MONTH,
  YEAR,
  BRIGHTNESS,
  START,
  END,
  DISABLED,
};

// Button actions to be fed into configuration module
enum ActionType
{
  CONTINUE,
  FUNCTION,
  INCREASE,
  DECREASE,
};

// Active display state
DisplayState displayState = TIME;
// Active timer state
TimerState timerState = STOPPED;
// Active configuration state
SetState configurationState = DISABLED;

// Led Control object to control MAX72XX device
LedControl lc = LedControl(12, 11, 10, 1);
//RTC_DS3231 rtc;
RTC_Millis rtc;

char timeBuffer[4];
int tempBuffer[3];
int timerBuffer[4];
int configBuffer[4];

char displayBuffer[4];
boolean dotBuffer[4];

char configurationTag[] = {
    0,
    5,
    'L',
    'H',
    'd',
    'P',
    4,
    8,
};

// 1/2 Hz blink
boolean blink = false;

// Keeps track of seconds, used for FUNC_ON_MINUTE and reset inside
int second = 0;

// Keeps track of seconds to left on Temp screen
int timeOut = TEMP_TIMEOUT;

// Start of the timer.
DateTime timerStart = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  setButton.attach(BUTTON_SET_PIN);
  setButton.interval(DEBOUNCE);

  pinMode(BUTTON_FUNCTION_PIN, INPUT_PULLUP);
  functionButton.attach(BUTTON_FUNCTION_PIN);
  functionButton.interval(DEBOUNCE);

  pinMode(BUTTON_TEMP_BUTTON, INPUT_PULLUP);
  tempButton.attach(BUTTON_TEMP_BUTTON);
  tempButton.interval(DEBOUNCE);

  pinMode(BUTTON_TIMER_BUTTON, INPUT_PULLUP);
  timerButton.attach(BUTTON_TIMER_BUTTON);
  timerButton.interval(DEBOUNCE);

  lc.shutdown(0, false);
  lc.setIntensity(0, 1);
  lc.clearDisplay(0);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  second = rtc.now().second();
}
long preMillis = 0;
void loop()
{
  long p = millis();
  readButtons();
  if (p - preMillis >= 1000)
  {
    preMillis = p;
    FUNC_ON_SECOND();
  }
}

void FUNC_ON_SECOND()
{
  updateDisplay();
  printDisplay();
  blinkTimeDot();
  heartbeat();
  second++;
  if (second > 60)
  {
    FUNC_ON_MINUTE();
  }
}

void FUNC_ON_MINUTE()
{
  second = 0;
}

void readButtons()
{
  setButton.update();

  if (setButton.fell())
  {
    if (displayState == DisplayState::CONFIG)
    {
      writeSet(ActionType::CONTINUE);
    }
    else
    {
      displayState = DisplayState::CONFIG;
      configurationState = SetState::START;
    }
  }

  functionButton.update();

  if (functionButton.fell())
  {
    if (displayState == DisplayState::CONFIG)
    {
      writeSet(ActionType::FUNCTION);
    }
  }

  timerButton.update();

  if (timerButton.fell())
  {
    //First press
    //Enter the TIMER state
    if(displayState == DisplayState::TIME){
      displayState = DisplayState::TIMER;
      clearDisplayBuffer();
      timerState = TimerState::STOPPED;
    } 
    //Second press on TIMER Button, Timer Start
    else if (displayState == DisplayState::TIMER)
    {
      if(timerState == TimerState::STOPPED)
      {
        timerState = TimerState::RUNNING;
        timerStart = rtc.now();
      }
      else if(timerState == TimerState::RUNNING)
      {
        timerState = TimerState::PAUSED;
      } 
      else 
      {
        displayState = DisplayState::TIME;
      }
    }
    // First press
    // Inside Config state
    else if (displayState == DisplayState::CONFIG)
    {
      writeSet(ActionType::INCREASE);
    }
  }

  tempButton.update();

  if (tempButton.fell())
  {
    if (displayState == DisplayState::CONFIG)
    {
      writeSet(ActionType::DECREASE);
    }
    else
    {
      displayState = TEMP;
    }
  }
}

void updateDisplay()
{
  switch (displayState)
  {
  case TIME:
    writeTime();
    break;
  case TEMP:
    parseTemp();
    writeTemp();
    break;
  case CONFIG:
    writeSet(NULL);
    break;
  case TIMER:
    writeTimer();
    break;

  default:
    break;
  }
}

void writeTime()
{
  parseTime();
  dotBuffer[1] = blink;
  printDisplay();
}

void writeTemp()
{
  timeOut--;
  if (timeOut == 0)
  {
    timeOut = TEMP_TIMEOUT;
    displayState = TIME;
  }
}
int setTimeBuffer[6];
boolean leftDigit = true;
void writeSet(ActionType type)
{
  if (configurationState == START)
  {
    DateTime time = rtc.now();
    setTimeBuffer[0] = time.second();
    setTimeBuffer[1] = time.minute();
    setTimeBuffer[2] = time.hour();
    setTimeBuffer[3] = time.day();
    setTimeBuffer[4] = time.month();
    setTimeBuffer[5] = time.year() % 1000;
  }
  /*
  switch (type)
  {
  case ActionType::CONTINUE:
    if (configurationState == END)
    {
      displayState = TIME;
      rtc.adjust(DateTime(2000 + setTimeBuffer[5], setTimeBuffer[4], setTimeBuffer[3], setTimeBuffer[2], setTimeBuffer[1], setTimeBuffer[0]));
      configurationState = DISABLED;
    }
    configurationState = (SetState)configurationState + 1;
    break;
  case ActionType::FUNCTION:
    leftDigit = !leftDigit;
    break;
  case ActionType::INCREASE:
    break;
  case ActionType::DECREASE:
    break;
  }
  */

  displayBuffer[0] = configurationTag[configurationState];
  displayBuffer[1] = ' ';
  displayBuffer[2] = '0';
  displayBuffer[3] = '1';

  dotBuffer[2] = leftDigit;
  dotBuffer[3] = !leftDigit;
}

void printDisplay()
{
  lc.clearDisplay(0);
  lc.setChar(0, 0, displayBuffer[0], dotBuffer[0]);
  lc.setChar(0, 1, displayBuffer[1], dotBuffer[1]);
  lc.setChar(0, 2, displayBuffer[2], dotBuffer[2]);
  lc.setChar(0, 3, displayBuffer[3], dotBuffer[3]);
}

void writeTimer()
{
  if(timerState == TimerState::RUNNING){
   parseTimer();
  }
  dotBuffer[1] = (timerState == TimerState::RUNNING) && blink;
}

void blinkTimeDot()
{
  blink = !blink;
}

void parseTime()
{
  DateTime now = rtc.now();
  displayBuffer[0] = ((now.hour()) / 10);
  displayBuffer[1] = ((now.hour()) % 10);
  displayBuffer[2] = ((now.minute()) / 10);
  displayBuffer[3] = ((now.minute()) % 10);

}

void parseTemp()
{
  //rct.now();
  displayBuffer[0] = 1;
  displayBuffer[1] = 9;
  displayBuffer[2] = 6;
  displayBuffer[3] = tempChar;
  dotBuffer[3] = true;
}

void parseTimer()
{
  TimeSpan span = rtc.now() - timerStart;
  if (span.hours() == 0)
  {
    displayBuffer[0] = span.minutes() / 10;
    displayBuffer[1] = span.minutes() % 10;
    displayBuffer[2] = span.seconds() / 10;
    displayBuffer[3] = span.seconds() % 10;
  }
  else
  {
    displayBuffer[0] = span.hours() / 10;
    displayBuffer[1] = span.hours() % 10;
    displayBuffer[2] = span.minutes() / 10;
    displayBuffer[3] = span.minutes() % 10;
  }
}

boolean beat = false;
void heartbeat()
{
  digitalWrite(LED_BUILTIN, beat ? HIGH : LOW);
  beat = !beat;
}

void clearDisplayBuffer(){
  displayBuffer[0] = 0;
  displayBuffer[1] = 0;
  displayBuffer[2] = 0;
  displayBuffer[3] = 0;
  dotBuffer[0] = false;
  dotBuffer[1] = false;
  dotBuffer[2] = false;
  dotBuffer[3] = false;
}