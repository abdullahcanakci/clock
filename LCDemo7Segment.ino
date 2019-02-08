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
#define TEMP_CHAR 'f'
#define TEMP_TIMEOUT 15;
#define BUTTON_HOUR_PIN 2
#define BUTTON_MINUTE_PIN 3
#define BUTTON_TEMP_BUTTON 4
#define BUTTON_TIMER_BUTTON 5
#define DEBOUNCE 25

char tempChar = TEMP_CHAR;

Bounce hourButton = Bounce();
Bounce minuteButton = Bounce();
Bounce tempButton = Bounce();
Bounce timerButton = Bounce();

//SOFTWARE

enum DisplayState{
  TIME,
  TEMP,
  SET,
  TIMER,
};

enum TimerState {
  STOPPED,
  PAUSED,
  RUNNING,
};

DisplayState displayState = TIME;
TimerState timerState = STOPPED;
 
LedControl lc=LedControl(12,11,10,1);
//RTC_DS3231 rtc;
RTC_Millis rtc;

int timeBuffer[4];
int tempBuffer[3];
int timerBuffer[4];

boolean blink = false;

int second = 0;
int timeOut = TEMP_TIMEOUT;

DateTime timerStart = 0;
boolean timerRunning = false;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BUTTON_HOUR_PIN, INPUT_PULLUP);
  hourButton.attach(BUTTON_HOUR_PIN);
  hourButton.interval(DEBOUNCE);

  pinMode(BUTTON_MINUTE_PIN, INPUT_PULLUP);
  minuteButton.attach(BUTTON_MINUTE_PIN);
  minuteButton.interval(DEBOUNCE);

  pinMode(BUTTON_TEMP_BUTTON, INPUT_PULLUP);
  tempButton.attach(BUTTON_TEMP_BUTTON);
  tempButton.interval(DEBOUNCE);

  pinMode(BUTTON_TIMER_BUTTON, INPUT_PULLUP);
  timerButton.attach(BUTTON_TIMER_BUTTON);
  timerButton.interval(DEBOUNCE);

  lc.shutdown(0,false);
  lc.setIntensity(0,2);
  lc.clearDisplay(0);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  second = rtc.now().second();
}
long preMillis = 0;
void loop() {
  long p = millis();

  readButtons();  
  if(p - preMillis >= 1000){
    preMillis = p;
    FUNC_ON_SECOND();
  }
  
}

void FUNC_ON_SECOND(){
  blinkTimeDot();
  updateDisplay();
  heartbeat();
  second++;
  if(second > 60){
    FUNC_ON_MINUTE();
  }
}

void FUNC_ON_MINUTE() {
  second = 0;
}

void readButtons(){
  hourButton.update();

  if(hourButton.fell()){
    Serial.println("Button press");
  }

  minuteButton.update();

  if(minuteButton.fell()){

  }

  timerButton.update();

  if(timerButton.fell()){
    if(displayState == TIMER){

      //PAUSE TIMER AND HOLD
      if(timerState == RUNNING) {
        timerState == PAUSED;
      } 
      else if (timerState == PAUSED) {
        timerState = STOPPED;
        displayState = TIME;
      } 
      else {
        timerState = RUNNING;
        timerStart = rtc.now(); 
      }
    } else {
      displayState = TIMER;
    }
  }

  tempButton.update();

  if(tempButton.fell()){
    displayState = TEMP;
  }
}

void updateDisplay(){
  switch (displayState)
  {
    case TIME:
      parseTime();
      lc.clearDisplay(0);
      writeTime();
      break;
    case TEMP:
      parseTemp();
      lc.clearDisplay(0);
      writeTemp();
      break;
    case SET:
      writeSet();
      break;
    case TIMER:
      writeTimer();
      break;
  
    default:
      break;
  }

}


void writeTime() {
  for(int i = 0; i < 4; i++){
    lc.setDigit(0, i, timeBuffer[i], i == 1 && blink);
  } 
}

void writeTemp() {
  lc.setDigit(0,0,tempBuffer[0], false);
  lc.setDigit(0,1,tempBuffer[1], true);
  lc.setDigit(0,2,tempBuffer[2], false);
  lc.setChar(0,3,TEMP_CHAR, true);

  timeOut--;
  if(timeOut == 0){
    timeOut = TEMP_TIMEOUT;
    displayState = TIME;
  }
}

void writeSet() {

}

void writeTimer() {
  if(timerState == RUNNING){
    parseTimer();
  }
  lc.clearDisplay(0);
  lc.setDigit(0,0,timerBuffer[0], false);
  lc.setDigit(0,1,timerBuffer[1], timerRunning == RUNNING && blink);
  lc.setDigit(0,2,timerBuffer[2], false);
  lc.setDigit(0,3,timerBuffer[3], false);
}

void blinkTimeDot(){
  blink = !blink;
}

void parseTime(){
  DateTime now = rtc.now();
  timeBuffer[0] = (now.hour()) / 10;
  timeBuffer[1] = (now.hour()) % 10;
  timeBuffer[2] = (now.minute()) / 10;
  timeBuffer[3] = (now.minute()) % 10;
}

void parseTemp() {
  //rct.now();
  tempBuffer[0] = 1;
  tempBuffer[1] = 9;
  tempBuffer[2] = 6;
}

void parseTimer() {
  TimeSpan span = rtc.now() -timerStart;
  if(span.hours() == 0){
    timerBuffer[0] = span.minutes() / 10;
    timerBuffer[1] = span.minutes() % 10;
    timerBuffer[2] = span.seconds() / 10;
    timerBuffer[3] = span.seconds() % 10;
  } else {
    timerBuffer[0] = span.hours() / 10;
    timerBuffer[1] = span.hours() % 10;
    timerBuffer[2] = span.minutes() / 10;
    timerBuffer[3] = span.minutes() % 10;
  }
}

boolean beat = false;
void heartbeat(){
  digitalWrite(LED_BUILTIN, beat ? HIGH : LOW);
  beat = !beat;
}

