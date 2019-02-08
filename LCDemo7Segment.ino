//We always have to include the library
#include "LedControl.h"
#include <Wire.h>
#include "RTClib.h"

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
#define NUMBER_OF_DIGITS 4
#define BUTTON_HOUR_PIN 2
#define BUTTON_MINUTE_PIN 3
#define BUTTON_TEMP_BUTTON 4
#define BUTTON_TIMER_BUTTON 5
#define DEBOUNCE 50
#define HOLD_TIME 2000


//HARDWARE 
int buttonState = 0;
int lastButtonState = 0;
int lastDebounceTime = 0;


//SOFTWARE

enum DisplayState{
  TIME,
  TEMP,
  SET,
};

DisplayState displayState = TIME;
 
LedControl lc=LedControl(12,11,10,1);
//RTC_DS3231 rtc;
RTC_Millis rtc;

int timeBuffer[4];

boolean isValid = false;

boolean blink = false;


/* we always wait a bit between updates of the display */
unsigned long delaytime=250;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_HOUR_PIN, INPUT);

  lc.shutdown(0,false);
  lc.setIntensity(0,4);
  lc.clearDisplay(0);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  

  parseTime();
}
long preMillis = 0;
void loop() {
  long p = millis();

  int reading = digitalRead(BUTTON_HOUR_PIN);

  
  //Button PRESSED
  if(reading != lastButtonState){
    lastDebounceTime = p;
  }
  if((p - lastDebounceTime) > DEBOUNCE){
    if(reading != buttonState){
      buttonState = reading;

      if(buttonState == HIGH){
        Serial.println("Button pressed");
      }
    }
  }
  

  if(p - preMillis >= 1000){
    preMillis = p;
    FUNC_ON_SECOND();
  }
  
}

void FUNC_ON_50_MILLI(){
  
}

void FUNC_ON_100_MILLI() {
  
}

void FUNC_ON_250_MILLI() {
  
}

void FUNC_ON_500_MILLI() {

}

void FUNC_ON_SECOND(){
  blinkTimeDot();
  updateDisplay();
  heartbeat();
  parseTime();
}

void FUNC_ON_MINUTE() {
}

void updateDisplay(){
  lc.clearDisplay(0);
  switch (displayState)
  {
    case TIME:
      writeTime();
      break;
    case TEMP:
      writeTemp();
      break;
    case SET:
      writeSet();
  
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

}

void writeSet() {

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

boolean beat = false;
void heartbeat(){
  digitalWrite(LED_BUILTIN, beat ? HIGH : LOW);
  beat = !beat;
}

