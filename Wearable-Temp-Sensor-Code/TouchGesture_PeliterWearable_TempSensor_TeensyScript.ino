// 9-17-2015 Grand Challenge Teensy version
// Wearable capacitive controlled heating/cooling peltier system
// Introduces a pull and rub garment gesture to indicate feeling hot and cold
// Once discomfort is recorded the peltier system heats up or cools the user's wrist. 


#include <avr/pgmspace.h>
#include <Wire.h>

volatile char state;                       //Program State Setup 

//Timer Setup
const int countInterval = 5000;           //time to register additional button presses1000 mS/S * 10 S
const int recInterval = 10000;             //time to heatin/cool 1000 mS/S * 60 S/min * 10 mins
const int pressInterval = 5000;            // time to recognize first pressed buttons

//Button Setup
volatile int buttonCount = 0;             // initialize vi count to start at zero

int sensorValue = 0;                      //variable for sensor reading
int ignore = 0;                           //ignore counter to ignore interrupts

//Pin Setup
const int vib = 0;                        // Lilypad Vibrator pin
const int thermistor = 19;              // Thermistor pin

String dataString;
String cthermalstate;                    //thermal state string [Warm, Hot, Cool, Cold, Netural]
String gpsloc;

volatile int intevent = 0;               //event counter "has button been pressed"

//jst thermistor
//float c1=.001468, c2=.0002383 , c3=.0000001007;
float c1=.0008236945712, c2=.0002632511198 , c3=.0000001349588752;  //Steinhart Coefficient for 103JT-025

//Capbutton Setup
//cap setup
long starttime;
long starttime2;
long stotal1, stotal2, stotal3;
long currenttime;
long duration = 5000;
int upCount, downCount, pullCount;

void setup() {
  // Initialize Serial Communications at 9600 bps:  
  Serial.begin(9600);

  //Vibrator Setup  
  pinMode(vib, OUTPUT);                               //pin setup for Lilypad Vibrator
  state='R';                                          //Initialize R-recording state on start up
  pinMode(thermistor, INPUT);                                 //Setup A6 pin to read temp sensor
  Serial.print("Count Interval (mS): ");
  Serial.println(countInterval);
  Serial.print("Record Interval (mS): ");
  Serial.println(recInterval);

  bcintstat();
  intevent=0;                                      //Resets intevent since it gets tripped up initialization....  

  //H-Bridge Setup
  pinMode(10, OUTPUT);  //pin 12 is Enable
  pinMode(9, OUTPUT);  //Pin10 is 2A (- side of thermoelec)
  pinMode(11, OUTPUT);  //Pin11 is 1A (+ side of thermoelec) (toggle 1A/2A to flip polarity)
  pinMode(17, OUTPUT);  //VCC
  digitalWrite(17,HIGH);
  pinMode(13, OUTPUT);  //light 
  digitalWrite(13,HIGH);  //light
 
  boolean start= 1;

}

void loop()  { 
  switch (state){
    
  case 'R':      //Data Recording State
    Serial.println("Data Recording State");

    comfortgesture();
    analogWrite(10, 0);        //Shut Off Peltier          

    if (intevent == 1){                         //If button was pressed
      Serial.println(F("Button Was Pressed"));
      intevent=0;            //Reset event counter
      bcintstat();
      break;
    }
    else {                                    //If button wasn't pressed
      cthermalstate = "Neutral";            //Body is in neutral
      gpsloc="-";
      dataRec(2);                           //Type in # of sensors to poll, start at 
      moddelay(500);                          //Delay X seconds before recording next data
      break;                
    }

  case 'X':                                  //Transition State
    intevent=0;                            //Reset intevent counter
    Serial.println(("Transition State"));
    counttimer(countInterval,1);                    //mode 1 - counttimer immediately breakout 
    if (intevent == 0) {                            // No button was pressed
      state='R';                                    //no button pressed return to recording state
      Serial.println(F("No button pressed. Returning to Neutral Record State"));
      delay(500);
      intevent = 0;
      buttonCount = 0;
      bcintstat();
      break;
    }  
    else if (buttonCount > 0)  {                                        //Button was pressed so go to Counting State
      intevent = 0;
      bcintstat();
      state='C';
      Serial.println(F("Button pressed, returning to Button Counting State"));
      break;
    }

  case 'C':                                                             //Button Counting State
    intevent = 0;                                                     //Reset event counter
    Serial.println("Button Counting State");
    counttimer(countInterval,0);                                      //Enter button count timer function
    state = 'A';                                                      //After countInterval switch to Analyze state
    break;

  case 'A':                                                              //Analyze button count state
    Serial.println("Analyze Mode");
    bcintstat();
    
    if (buttonCount == 0) {                                            // Leaving WiFi or Thermal State Recording, then go back to record loop
      Serial.println();
      Serial.println(F("Transitioning to Thermal Recording State: Netural")); 
      state ='R';
      intevent=0;
      bcintstat();
      buttonCount=0;
      break;
    }

    else if (buttonCount == -1)  {                              //1-Down Swipe = Cool 
      Serial.println(F("Transitioning to Thermal Recording State: Cool"));
      cthermalstate="Cool";
      state = 'T';
      intevent=0;
      bcintstat();
      break;
    }

    else if (buttonCount == -2) {                                //2-Down Swipe = Cold
      Serial.println(F("Transitioning to Thermal Recording State: Cold"));
      cthermalstate="Cold";
      state = 'T';
      intevent=0;
      bcintstat();
      break;
    }

    else if (buttonCount == 1) {                                    //1-Up Swipe = Warm
      Serial.println(F("Transitioning to Thermal Recording State: Warm"));
      cthermalstate="Warm";
      state = 'T';
      intevent=0;
      bcintstat();
      break;
    } 

    else if (buttonCount == 2) {                                    //2-Up Swipe = Hot 
      Serial.println(F("Transitioning to Thermal Recording State: Hot"));
      cthermalstate="Hot";
      state = 'T';
      buttonCount=0;
      bcintstat();
      intevent=0;
      break;
    }

    else if (buttonCount >= 100) {                                  //Swipe Left = WiFi recording
      Serial.println(F("WiFi Mode"));
      state = 'W';
      break;
    }

    else if (buttonCount <= -10) {                                  //Far Swipe = Reset
      Serial.println(F("Errant Button Pressed, RESET back to Netural Recording"));
      state = 'R';
      intevent= 0 ;
      buttonCount= 0 ;
      bcintstat();
      break;
    }  

    else {                                                          //Too many button presses? Return to Counting Mode
      Serial.println(F("ERROR too many button presses, indicate comfort again"));
      vibsignal(vib,2000);                                          //long vibrate to indicate error and new input
      buttonCount = 0;
      intevent = 0;
      state = 'C';
      bcintstat();
      break;
    }     

  case 'W':                                                        //WiFi Button State  (which can only occur during neutral condition)
    //Wifi Code
    vibsignal(vib,2000);                                           //vibrate to indicate input
    buttonCount = 0;                                               // reset button count
    intevent = 0;   
    state = 'C';         //vibrate signal new input
    break;

  case 'T':                                                        //Thermal Recording State
    Serial.println(F("Thermal Recording State"));
    gpsloc=String('-');
    trecording(recInterval);                // record thermal state for recording interval
    state = 'X';          //exit to transition state
    buttonCount = 0;                        //reset button count
    intevent = 0 ;
    vibsignal(vib,2000);                    //vibrate to indicate input
    break;
  }  
}


//Functions

// FUNCTION TO RECORD DATA
void dataRec(int k) {                               //k, number of sensors starting at 0. 
  dataString.remove(0);                             //Clear string
  int time = 0;                                     //Save Time      
  dataString += String(time) + "," + String(intevent) + "," + String(buttonCount) + ",";

  for (int i=0; i < k-1; i++)  {                      // read the analog pin  
    sensorValue = int(analogRead(thermistor));
    //Serial.println(sensorValue);                  //analogread of thermistor
    float Rt=((1024.0/sensorValue)-1)*9750.0;
    //float voltage = sensorValue *3.96/1024.0;
    //float Rt = 9943.0*((3.96/voltage)-1);
    float logRt = log(Rt);
    float T = (1.0/(c1+c2*logRt+c3*logRt*logRt*logRt))-273.15;
    float Tf = (T*9.0/5.0)+32.0;
    //sensorValue = ((sensorValue*5/1024-.5)*100)*(9/5)+32;  
    dataString += String(Tf) + ",";
    //Serial.print(F("Sensor #"));
    //Serial.print(String(i));
    Serial.print(F("Skin temp is: "));
    Serial.print(Tf);
    Serial.println(" F.");
  }
  dataString += cthermalstate + "," + gpsloc;        // concatenatte the thermal state and gps location
  //Serial.println(dataString);
}    

//Counter Function during Thermal Recording Mode
void trecording(int interval){ 
  if (buttonCount > 0) {                        //setup peltier for cooling
    digitalWrite(13,LOW);  //cooling mode blink once
    delay  (500);
    digitalWrite(13,HIGH); 
    
    digitalWrite(17,HIGH);
    analogWrite(10,1023);
    digitalWrite(11,HIGH);
    digitalWrite(9,LOW);
    Serial.println("Cool Mode");
    buttonCount = 0;
  }
  else {                                      //setup peltier for heating
    digitalWrite(13,LOW);  //heating mode blink twice
    delay(500);
    digitalWrite(13,HIGH);  
    delay(500);
    digitalWrite(13,LOW);  
    delay(500);
    digitalWrite(13,HIGH);  
    
    digitalWrite(17,HIGH);
    analogWrite(10,500);
    digitalWrite(11,LOW);
    digitalWrite(9,HIGH);
    Serial.println("Heat Mode");
    buttonCount = 0;
  }  
  int startTime=millis();                      //reset start time
  int currentTime=millis();                    //reset current time
  int elapsedTime=(currentTime-startTime);
  while (elapsedTime < interval) {             // If current time - start time is greater than set recorded interval, continue data collection
    dataRec(2);                                // Record Thermal State
    currentTime=millis(); 
    elapsedTime=(currentTime-startTime);
    Serial.print(F("Elapsed Time mS: "));
    Serial.println(elapsedTime);
    delay(1000);                                //Delay to control how often to record data 
  }
  analogWrite(10, 0);                         //Turn off peltier
  digitalWrite(11, LOW);                         //Turn off peltier
  digitalWrite(9, LOW);
}

void moddelay(int duration) {
  int startTime=millis();
  int currentTime=millis();
  int elapsedTime=(currentTime-startTime);
  while (elapsedTime < duration){      
    delay(100);                                     //delay each iteration for 1 secs to wait for button press
    currentTime=millis();
    elapsedTime=(currentTime-startTime);
    if (intevent > 0) {                //button was pressed 
      elapsedTime=duration+1;
      bcintstat();
      break;
    }
  }
}



//Functiong to count button presses during Count Mode
void counttimer(int interval, int mode)  {          //mode = 0 is for continuous recording, mode = 1 is to break immediately if button is pressed.
  int startTime=millis();
  int currentTime=millis();
  int elapsedTime=(currentTime-startTime);

  while (elapsedTime < interval){      
    delay(1000);                                     //delay each iteration for 1 secs to wait for button press
    currentTime=millis();
    Serial.print(F("Elapsed Time mS: "));
    elapsedTime=(currentTime-startTime);
    Serial.println(elapsedTime);  
    //bcintstat();
    comfortintensitycount();        
    if (intevent > 0) {                //button was pressed 
      if (mode == 0) {                //For continous mode, reset event and continue counting buttons until set time expires (increment button count)
        intevent=0;
        bcintstat();
      }
      else if (mode == 1) {          //In mode 1, once button is pressed, we break out of loop, and assess the gesture (increment button and event count)
        elapsedTime = interval+1;
        break;
      }  
    }
  }
}

void vibsignal(int indicator, int timems){
  digitalWrite(indicator, HIGH);                //vibrate to indicate input
  delay(timems);
  digitalWrite(indicator, LOW); 
}

void lightsignal(int indicator, int timems){
  analogWrite(indicator, 255);                //vibrate to indicate input
  //detachInterrupt(0);
  delay(timems);
  //attachInterrupt(0, interruptRoutine, FALLING);
  analogWrite(indicator, 0); 
}

void bcintstat()  {
  Serial.print(F("Button Count: "));
  Serial.println(buttonCount);
  Serial.print(F("Intevent: "));
  Serial.println(intevent);
}

void comfortgesture() {
  long total1 =  touchRead(16);   // Down Button
  long total2 =  touchRead(15);   // Up Button
  long total3 =  touchRead(18);   // Pull Button
  //Serial.print("Cap values: ");
  //Serial.print(total1);                  // print sensor output 1
  //Serial.print(",");
  //Serial.print(total2);                  // print sensor output 2
  //Serial.print(",");
  //Serial.println(total3);                  // print sensor output 3

    if(total1 > 3000 ){                      //Check threshhold for Down Button
    //Serial.println("DOWN");
    downCount = downCount+1;
  }

  if(total2 > 3000){                        //Check threshhold for Up Button
    //Serial.println("UP");
    upCount=upCount+1;
  }

  if(total3 < 2000){                        //Check threshhold for Pull Button
    //Serial.println("Pull");
    pullCount=pullCount+1;
  }

  Serial.println();
  Serial.print("Counter:");
  Serial.print(downCount);
  Serial.print(",");
  Serial.print(upCount);
  Serial.print(",");
  Serial.println(pullCount); 

  if ((upCount == 1 || downCount == 1 ) && starttime == 0){
    starttime = millis();                  //start time at 1st button press   
  }

  if (starttime > 0) {
    Serial.print("Time: ");
    Serial.println(millis()-starttime);

    if (millis()-starttime > duration){
      if (upCount > 3 && downCount > 3){
        starttime = 0;
        upCount = 0;
        downCount = 0;
        Serial.println("%%%%%IM COLD%%%%");
        state='C';
        buttonCount += -1;
        intevent += 1;
        vibsignal(vib,1000);
      }
      else{
        starttime=0;
        upCount=0;
        downCount=0;
        Serial.println(" not enough presses");  
      }
    }      
  }

  if (pullCount == 1 && starttime2==0) {
    starttime2 = millis();      //start time at 1st button press
    Serial.println(starttime2);
  }
  if (starttime2 > 0) {
    Serial.print("Time2: ");
    Serial.println(millis()-starttime2);

    if (millis()-starttime2 > duration){
      if (pullCount > 5){
        starttime2 = 0;
        pullCount = 0;
        Serial.println("%%%IM HOT%%%");
        state='C';
        buttonCount += 1;
        intevent += 1;
        vibsignal(vib,1000);
      }
      else{
        starttime2=0;
        pullCount=0;
        Serial.println(" not enough pulls");  
      }  
    }      
  }
}

void comfortintensitycount(){
  long total1 =  touchRead(16);   // Down Button
  long total2 =  touchRead(15);   // Up Button

  Serial.print("Cap values: ");
  Serial.print(total1);                  // print sensor output 1
  Serial.print(",");
  Serial.println(total2);                  // print sensor output 2    

    if(total1 > 2500 ){                      //Check threshhold for Down Button
    Serial.println("DOWN");
    buttonCount += -1;
    intevent += 1;
    vibsignal(vib,1000);
  }

  if(total2 > 2600){                        //Check threshhold for Up Button
    Serial.println("UP");
    buttonCount += 1;
    intevent += 1;
    vibsignal(vib,1000);
  }
}



