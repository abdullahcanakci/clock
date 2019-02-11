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

int brightness = 4;
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
enum ConfigurationState
{
  SECOND,
  MINUTE,
  HOUR,
  DAY,
  MONTH,
  YEAR,
  BRIGHTNESS,
  END,
  START,
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
ConfigurationState configurationState = DISABLED;

// Led Control object to control MAX72XX device
LedControl lc = LedControl(12, 11, 10, 1);
//RTC_DS3231 rtc;
RTC_Millis rtc;

char displayBuffer[4];
boolean dotBuffer[4];

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
  lc.setIntensity(0, brightness);
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
  //heartbeat();
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
    if (displayState == DisplayState::TIME)
    {
      displayState = DisplayState::CONFIG;
      writeSet(ActionType::CONTINUE);
    }
    else if (displayState == DisplayState::CONFIG)
    {
      writeSet(ActionType::CONTINUE);
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
    if (displayState == DisplayState::TIME)
    {
      displayState = DisplayState::TIMER;
      clearDisplayBuffer();
      timerState = TimerState::STOPPED;
    }
    //Second press on TIMER Button, Timer Start
    else if (displayState == DisplayState::TIMER)
    {
      if (timerState == TimerState::STOPPED)
      {
        timerState = TimerState::RUNNING;
        timerStart = rtc.now();
      }
      else if (timerState == TimerState::RUNNING)
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
  case DisplayState::TIME:
    writeTime();
    break;
  case DisplayState::TEMP:
    parseTemp();
    writeTemp();
    break;
  case DisplayState::TIMER:
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
  dotBuffer[2] = blink;
  //printDisplay();
}

void writeTimer()
{
  if (timerState == TimerState::RUNNING)
  {
    parseTimer();
  }
  dotBuffer[1] = (timerState == TimerState::RUNNING) && blink;
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

char configurationTag[] = {
    '5',
    '0',
    'H',
    'd',
    'A',
    '4',
    '8',
};
//Second Minute Hour Day Month Year Brightness
int setTimeBuffer[7];
int configurationOverflow[] = {60, 60, 24, 32, 13, 100, 16};
boolean hasConfigurationChanged[] = {false, false, false, false, false, false, false};
boolean originalConfigurationValue = -1;
boolean leftDigit = true;
void writeSet(ActionType type)
{
  Serial.println("Write set " + type);

  if(type == ActionType::CONTINUE){
    //First entry
    if(configurationState == ConfigurationState::DISABLED){
    clearDisplayBuffer();
    DateTime time = rtc.now();
    setTimeBuffer[0] = time.second();
    setTimeBuffer[1] = time.minute();
    setTimeBuffer[2] = time.hour();
    setTimeBuffer[3] = time.day();
    setTimeBuffer[4] = time.month();
    setTimeBuffer[5] = time.year() % 1000;
    setTimeBuffer[6] = brightness;
    configurationState = ConfigurationState(-1);
    displayState = DisplayState::CONFIG;
    displayBuffer[0] = 'e';
    displayBuffer[1] = 'd';
    displayBuffer[2] = '1';
    displayBuffer[3] = '7';
    return;
    }
    if(type == ActionType::CONTINUE){
      if(originalConfigurationValue != -1 && originalConfigurationValue != setTimeBuffer[configurationState]){
        hasConfigurationChanged[configurationState] = true;
      }
      configurationState = ConfigurationState(configurationState + 1);
      originalConfigurationValue = setTimeBuffer[configurationState];
    }
    if (configurationState == ConfigurationState::END){

      configurationState = ConfigurationState::DISABLED;
      displayState = DisplayState::TIME;
      brightness = setTimeBuffer[6];
      lc.setIntensity(0, setTimeBuffer[6]);
      clearDisplayBuffer();
      DateTime end = rtc.now();

      //We are starting set mode with a fixed time if we use that and transfer it to the clock, it will be always behind
      //To fix this, we are taking a new time and create a comlex structure between to where user edited settings.
      DateTime complex = DateTime(
        hasConfigurationChanged[5] ? (setTimeBuffer[5] + 2000) : end.year(),
        hasConfigurationChanged[4] ? setTimeBuffer[4] : end.month(),
        hasConfigurationChanged[3] ? setTimeBuffer[3] : end.day(),
        hasConfigurationChanged[2] ? setTimeBuffer[2] : end.hour(),
        hasConfigurationChanged[1] ? setTimeBuffer[1] : end.minute(),
        hasConfigurationChanged[0] ? setTimeBuffer[0] : end.second()
      );
      rtc.adjust(complex);
      originalConfigurationValue = -1;
            for(int i = 0; i < 7; i++){
        hasConfigurationChanged[i] = false;
        Serial.println(setTimeBuffer[i]);
      }
      return;
    }
      
  }

  
  if (type == ActionType::FUNCTION){
    leftDigit = !leftDigit;
  } else if (type == ActionType::INCREASE){
      Serial.println("Increase button");
      int newValue = setTimeBuffer[configurationState];
      newValue += leftDigit == true ? 10 : 1;
      newValue = newValue % configurationOverflow[configurationState];
      setTimeBuffer[configurationState] = newValue;
  } else if (type == ActionType::DECREASE){
    Serial.println("Increase button");
      int newValue = setTimeBuffer[configurationState];
      newValue -= leftDigit == true ? 10 : 1;
      newValue += configurationOverflow[configurationState];
      newValue = newValue % configurationOverflow[configurationState];
      setTimeBuffer[configurationState] = newValue;
  }

    displayBuffer[0] = configurationTag[configurationState];
    displayBuffer[1] = ' ';
    displayBuffer[2] = setTimeBuffer[configurationState] / 10;
    displayBuffer[3] = setTimeBuffer[configurationState] % 10;

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

void blinkTimeDot()
{
  blink = !blink;
}

//Parses and feeds the time into display buffer
void parseTime()
{
  DateTime now = rtc.now();
  displayBuffer[0] = ((now.hour()) / 10);
  displayBuffer[1] = ((now.hour()) % 10);
  displayBuffer[2] = ((now.minute()) / 10);
  displayBuffer[3] = ((now.minute()) % 10);
}

//Parses temp and feeds into displayBuffer and manages conversion of units
void parseTemp()
{
  //rct.now();
  displayBuffer[0] = 1;
  displayBuffer[1] = 9;
  displayBuffer[2] = 6;
  displayBuffer[3] = tempChar;
  dotBuffer[3] = true;
}

//Calculates timer span and parses into displayBuffer
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

//Clears displayBuffer before new data enters
void clearDisplayBuffer()
{
  displayBuffer[0] = 0;
  displayBuffer[1] = 0;
  displayBuffer[2] = 0;
  displayBuffer[3] = 0;
  dotBuffer[0] = false;
  dotBuffer[1] = false;
  dotBuffer[2] = false;
  dotBuffer[3] = false;
}