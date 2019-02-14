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
#define TEMP_CHAR 'C'
#define TEMP_TIMEOUT 15;
#define BUTTON_SET_PIN 2 //
#define BUTTON_FUNCTION_PIN 3
#define BUTTON_TEMP_BUTTON 4  //WILL BE USED FOR  DECREASE
#define BUTTON_TIMER_BUTTON 5 //WILL BE USED FOR INCREASE
#define LDR_PIN A0
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
RTC_DS3231 rtc;
//RTC_Millis rtc;

//Buffers for display
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

//Configuration Fields
//Tags for config states These should be  from below by LedControl Library
/*
*	'0','1','2','3','4','5','6','7','8','9','0',
*  'A','b','C','d','E','F','H','L','P',
*  '.','-','_',' '
*/
char configurationTag[] = {
    '5', //Second
    'L', //Minute
    'H', //Hour
    'd', //Day
    'A', //Month
    '4', //Year
    '8', //Brightness
};
//Second Minute Hour Day Month Year Brightness
int configurationBuffer[7];
//Overflow values by field
int configurationOverflow[] = {60, 60, 24, 32, 13, 100, 16};
//State change tracker.
boolean hasConfigurationChanged[] = {false, false, false, false, false, false, false};
//State holder between config cycles
boolean originalConfigurationValue = -1;
//7 segment dot value. Left means MSB for decade system
boolean leftDigit = true;

void setup()
{
  Serial.begin(9600);
  delay(500);

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

  pinMode(LDR_PIN, INPUT);

  //Chip is disabled at boot need to enable
  lc.shutdown(0, false);
  lc.setIntensity(0, 6);
  lc.clearDisplay(0);
  if (!rtc.begin())
  {
    Serial.println("There is no RTC chip detected.");
    delay(500);
    while (1)
      ;
  }

  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //DS3231 has lostPower register
    Serial.println("Seems like RTC chip lost power time set to flash time.");
  }
  else
  {

    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    Serial.println("Time is:");
    DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
  //To sychorinze minute operations with clock chip
  second = rtc.now().second();
}

long preMillis = 0;
void loop()
{
  long p = millis();
  readButtons();
  FUNC_ON_10_MILLI();
  if (p - preMillis >= 1000)
  {
    preMillis = p;
    FUNC_ON_SECOND();
    Serial.println(millis() - p, DEC);
  }
}

// General display update, For better response time and lower bandwith usage
void FUNC_ON_10_MILLI()
{
  updateDisplay();
  printDisplay();
}

void FUNC_ON_SECOND()
{
  blinkTimeDot();
  second++;
  if (second >= 60)
  {
    FUNC_ON_MINUTE();
  }
  int value = analogRead(LDR_PIN);
  if(value != brightness){
    brightness = value;
    lc.setIntensity(0, map(value, 0, 1100, 15, 0));
  }
}

void FUNC_ON_MINUTE()
{
  second = rtc.now().second(); //We are using internal clock to roughly estimate minute operations but they may be off -not much- we are syncing it here
}

void readButtons()
{
  setButton.update();

  if (setButton.fell())
  {
    writeSet(ActionType::CONTINUE);
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
  }
}

void writeTime()
{
  parseTime();
  dotBuffer[1] = blink;
  dotBuffer[2] = blink;
}

void writeTimer()
{
  parseTimer();
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

void writeSet(ActionType type)
{

  if (type == ActionType::CONTINUE)
  {
    //First entry
    if (configurationState == ConfigurationState::DISABLED)
    {
      clearDisplayBuffer();
      DateTime time = rtc.now();
      configurationBuffer[0] = time.second();
      configurationBuffer[1] = time.minute();
      configurationBuffer[2] = time.hour();
      configurationBuffer[3] = time.day();
      configurationBuffer[4] = time.month();
      configurationBuffer[5] = time.year() % 1000;
      configurationBuffer[6] = brightness;
      configurationState = ConfigurationState(-1);
      displayState = DisplayState::CONFIG;
      displayBuffer[0] = 'e';
      displayBuffer[1] = 'd';
      displayBuffer[2] = '1';
      displayBuffer[3] = '7';
      return;
    }
    if (type == ActionType::CONTINUE)
    {
      if (originalConfigurationValue != -1 && originalConfigurationValue != configurationBuffer[configurationState])
      {
        hasConfigurationChanged[configurationState] = true;
      }
      configurationState = ConfigurationState(configurationState + 1);
      originalConfigurationValue = configurationBuffer[configurationState];
    }
    if (configurationState == ConfigurationState::END)
    {

      configurationState = ConfigurationState::DISABLED;
      displayState = DisplayState::TIME;
      brightness = configurationBuffer[6];
      lc.setIntensity(0, configurationBuffer[6]);
      clearDisplayBuffer();
      DateTime end = rtc.now();

      //We are starting set mode with a fixed time if we use that and transfer it to the clock, it will be always behind
      //To fix this, we are taking a new time and create a comlex structure between to where user edited settings.
      DateTime complex = DateTime(
          hasConfigurationChanged[5] ? (configurationBuffer[5] + 2000) : end.year(),
          hasConfigurationChanged[4] ? configurationBuffer[4] : end.month(),
          hasConfigurationChanged[3] ? configurationBuffer[3] : end.day(),
          hasConfigurationChanged[2] ? configurationBuffer[2] : end.hour(),
          hasConfigurationChanged[1] ? configurationBuffer[1] : end.minute(),
          hasConfigurationChanged[0] ? configurationBuffer[0] : end.second());
      rtc.adjust(complex);
      originalConfigurationValue = -1;
      for (int i = 0; i < 7; i++)
      {
        hasConfigurationChanged[i] = false;
      }
      return;
    }
  }

  if (type == ActionType::FUNCTION)
  {
    leftDigit = !leftDigit;
  }
  else if (type == ActionType::INCREASE)
  {
    int newValue = configurationBuffer[configurationState];
    newValue += leftDigit == true ? 10 : 1;
    newValue = newValue % configurationOverflow[configurationState];
    configurationBuffer[configurationState] = newValue;
  }
  else if (type == ActionType::DECREASE)
  {
    int newValue = configurationBuffer[configurationState];
    newValue -= leftDigit == true ? 10 : 1;
    newValue += configurationOverflow[configurationState];
    newValue = newValue % configurationOverflow[configurationState];
    configurationBuffer[configurationState] = newValue;
  }

  displayBuffer[0] = configurationTag[configurationState];
  displayBuffer[1] = ' ';
  displayBuffer[2] = configurationBuffer[configurationState] / 10;
  displayBuffer[3] = configurationBuffer[configurationState] % 10;

  dotBuffer[2] = leftDigit;
  dotBuffer[3] = !leftDigit;
}

void printDisplay()
{
  //lc.clearDisplay(0);
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
  clearDisplayBuffer();
  DateTime now = rtc.now();
  displayBuffer[0] = ((now.hour()) / 10);
  displayBuffer[1] = ((now.hour()) % 10);
  displayBuffer[2] = ((now.minute()) / 10);
  displayBuffer[3] = ((now.minute()) % 10);
  dotBuffer[1] = blink;
  dotBuffer[2] = blink;
}

//Parses temp and feeds into displayBuffer and manages conversion of units
void parseTemp()
{
  clearDisplayBuffer();
  int temp = rtc.getTemp();
  Serial.print("Temp: ");
  Serial.println(temp, DEC);
  displayBuffer[0] = temp / 1000;
  temp = temp % 1000;
  displayBuffer[1] = temp / 100;
  temp = temp % 100;
  displayBuffer[2] = temp / 10;
  displayBuffer[3] = tempChar;
  dotBuffer[1] = true;
  dotBuffer[3] = true;
}

//Calculates timer span and parses into displayBuffer
void parseTimer()
{
  
  TimeSpan span = rtc.now() - timerStart;
  if (timerState == TimerState::STOPPED)
  {
    clearDisplayBuffer();
    displayBuffer[0] = 0;
    displayBuffer[1] = 0;
    displayBuffer[2] = 0;
    displayBuffer[3] = 0;
    return;
  }
  if (timerState == TimerState::RUNNING)
  {
    clearDisplayBuffer();
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
  dotBuffer[1] = timerState == TimerState::RUNNING && blink;
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
