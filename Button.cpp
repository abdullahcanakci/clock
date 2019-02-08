

public class Button {

public boolean checkButton(){
      //Button PRESSED
  if(reading != lastButtonState){
    lastDebounceTime = p;
  }
  if((p - lastDebounceTime) > DEBOUNCE){
    if(reading != buttonState){
      buttonState = reading;

      if(buttonState == HIGH){
          return true;
        Serial.println("Button pressed");
      }
    }
  }
}
}