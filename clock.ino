#include <LedControl.h>
#include <Wire.h>
#include <RTClib.h>
#include <Bounce2.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

/*
* MAX72XX
* Led driver
* pin 12 is connected to the DataIn
* pin 11 is connected to the CLK 
* pin 10 is connected to LOAD 
* We have only a single MAX72XX.
*/
/*
* DS18B20
* 1 wire tempature Sensor
* pin 6 is conected to te Data
*/
/*
* DS3231
* I2C RTC sensor with accuracy of 1 minute per year
* SDA to SDA
* SCL to SCL
*/
#define CLOCK_VERSION 1

#define NUMBER_OF_DIGITS 4
#define TEMPERATURE_UNIT 'C'
#define TEMP_TIMEOUT 15L // Time out for temperature display
#define DS18B20_ACCURACY 12
#define TEMP_READING_INTERVAL //DS18B20 has a problem when constant transmission might create temp rise on the chip and produce higher than normal value
#define BUTTON_SET_PIN 2 // 
#define BUTTON_FUNCTION_PIN 3 // USED FOR ALTERNATE ACTIONS
#define BUTTON_TEMP_BUTTON 4  // WILL BE USED FOR DECREASE
#define BUTTON_TIMER_BUTTON 5 // WILL BE USED FOR INCREASE
#define TEMP_SENSE_PIN 6
#define LDR_ENABLE true
#define LDR_PIN A0 //TO ADJUST BRIGHTNESS
#define MAX_BRIGHTNESS 15 //To be used in LDR brightness setting
#define MIN_BRIGHTNESS 1 //To be used in LDR brightness setting
#define DEBOUNCE 25

#define EEPROM_BRIGHTNESS_ADRESS 0

//Max brightness
int brightness = 15; 
char tempChar = TEMPERATURE_UNIT; //Cant provide C or F to the LedConrol from #define properties

/*
* BUTTONS *****************************
*/

Bounce setButton = Bounce();
Bounce functionButton = Bounce();
//ALT USE DECREASE
Bounce tempButton = Bounce();
// ALT USE INCREASE
Bounce timerButton = Bounce();

/*
* TEMPERATUR SET - UP *****************
*/
OneWire oneWire(TEMP_SENSE_PIN);
DallasTemperature tempSensor(&oneWire);

// Led Control object to control MAX72XX device
LedControl lc = LedControl(12, 11, 10, 1);
//rtc access object
RTC_DS3231 rtc; //Alternate chips can be used from Adafruit RTClib
//RTC_Millis rtc; //Can be used but it will roll over after ~48days

/*
* ENUMS *******************************
*/
//States of the Main display. Eventually their parse functions will be called to fill displayBuffer
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

/*
* States of configuration to go through
* We are iterating through this enum list. !!!
* Do not alter sorting without sorting relevant arrays
* configurationTag, configurationBuffer, configurationOverflow, hasConfigurationChanged
*/
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

//Buffers for display
char displayBuffer[4];
boolean dotBuffer[4];

// 1/2 Hz blink
boolean blink = false;

// Keeps track of seconds, used for FUNC_ON_MINUTE and reset inside
int second = 0;

// Keeps track of seconds to left on Temp screen
unsigned long timeOut = TEMP_TIMEOUT;
//DS18B20 sensors has a problem when under constant transmission it might heat up.
int tempReadingInterval = 200;
//Time of temp reading.
long previousTempReading = 0L;

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
  Serial.println("Boot started");
  Serial.print("Clock version");
  Serial.println(CLOCK_VERSION, DEC);

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

  if(LDR_ENABLE){
    pinMode(LDR_PIN, INPUT);
  }

  tempSensor.setResolution(DS18B20_ACCURACY);

  Serial.println("Buttons are wired up.");

  //Chip is disabled at boot need to enable
  lc.shutdown(0, false);
  if(!LDR_ENABLE){
    brightness = EEPROM.read(EEPROM_BRIGHTNESS_ADRESS);
    if(brightness == 0){
      brightness = 8;
    }
    lc.setIntensity(0, brightness);
  } else {
    updateDisplayBrightness();
  }
  lc.clearDisplay(0);

  Serial.println("Display init.");
  while(!rtc.begin()){
    Serial.println("There is no RTC chip detected. Try reconnecting in 1000ms");
    delay(1000);
  }

  //Some chips has lost power function. DS3231 is one of them.
  //If the battery has failed this will return true and we can use Flashing DateTime to set.
  // You may need to remove if for the first flash. My device didnt worked fine until I adjusted it.
  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //DS3231 has lostPower register
    Serial.println("Seems like RTC chip lost power time set to flash time.");
  }
  //To sychorinze minute operations with clock chip
  //We can use minute operations. - not used - sychorinzed with RTC clock
  second = rtc.now().second();
  Serial.println("Boot finished");
}

long preMillis = 0;
void loop()
{
  //This loop is taking 2-3 ms on 328p at 5V with 16Mhz clock speed while working 10_milli second and minute operation.
  //Need to lean it up but dont need it really buttons are not missed, display state changes are rapid etc.
  readButtons();
  FUNC_ON_10_MILLI();
  long p = millis();
  if (p - preMillis >= 1000L)
  {
    preMillis = millis();
    FUNC_ON_SECOND();
  }
  delay(1);
}

// General display update
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
  if (LDR_ENABLE)
  {
    updateDisplayBrightness();
  }
}

void FUNC_ON_MINUTE()
{
 second = rtc.now().second(); //We are using internal clock to roughly estimate minute operations but they may be off -not much- we are syncing it here
}

void updateDisplayBrightness(){
  int value = map(analogRead(LDR_PIN), 0, 1023, MAX_BRIGHTNESS, MIN_BRIGHTNESS);
  if(value != brightness){
    brightness = value;
    lc.setIntensity(0, brightness);
  }
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
      displayState = DisplayState::TEMP;
      previousTempReading = 0L;
      timeOut = millis() + 1000 * 15L; //plus 15 seconds
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
  if (timeOut < millis())
  {
    timeOut = 0L;
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
      if(!LDR_ENABLE){
        lc.setIntensity(0, brightness);
        EEPROM.update(EEPROM_BRIGHTNESS_ADRESS, brightness);
      }
      clearDisplayBuffer();
      DateTime endPoint = rtc.now();

      //We are starting set mode with a fixed time if we use that and transfer it to the clock, it will be always behind
      //To fix this, we are taking a new time and create a complex structure between to where user edited settings.
      DateTime complex = DateTime(
          hasConfigurationChanged[5] ? (configurationBuffer[5] + 2000) : endPoint.year(),
          hasConfigurationChanged[4] ? configurationBuffer[4] : endPoint.month(),
          hasConfigurationChanged[3] ? configurationBuffer[3] : endPoint.day(),
          hasConfigurationChanged[2] ? configurationBuffer[2] : endPoint.hour(),
          hasConfigurationChanged[1] ? configurationBuffer[1] : endPoint.minute(),
          hasConfigurationChanged[0] ? configurationBuffer[0] : endPoint.second());
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
  //Dont need to clear display we are writing everything
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
  //wait until ne 
  if(millis() - previousTempReading < tempReadingInterval){
    Serial.println("Passing temperature reading. This is not the time.");
    return;
  }
  //We cant just request and receive the temperature reading. It takes time on DS18B20 devices.
  if(!tempSensor.isConversionComplete()){
    return;
  }

  float t;
  if (TEMPERATURE_UNIT == 'C')
  {
    t = tempSensor.getTempCByIndex(0);
  }
  else
  {
    t = tempSensor.getTempFByIndex(0);
  }
  //Temperature is float but it is no bueno to parse
  //Multiplying because I want 2 significant bits to survive int conversion.
  //19.15 turn to 1915 and we can use regular int operations to parse it
  int temp = (int)(t * 100);

  //We received temp reading. Request a new one to display
  tempSensor.requestTemperaturesByIndex(0);

  clearDisplayBuffer();
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

  previousTempReading = millis();
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
