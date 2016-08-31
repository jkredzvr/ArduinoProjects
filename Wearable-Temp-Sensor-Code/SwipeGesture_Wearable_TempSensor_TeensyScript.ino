// 2015 Swipe Gesture Wearable Thermal Comfort System Data Logger 
// Uses IR sensor to recognize Up,Down,Left,Right swipes which are for indicating the user's thermal comfort
// in a more natural user interface.

#include <avr/pgmspace.h>
#include <SPI.h>  // Serial Peripheral Interface library
#include <SD.h>  // SD Card library
#include <ccspi.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

#define APDS9960_INT     2                 // interrupt pin
volatile char state;                       //Program State Setup 

//Timer Setup
const int countInterval = 5000;           //1000 mS/S * 10 S
const int recInterval = 4000;             //1000 mS/S * 60 S/min * 10 mins

//Button Setup
volatile int buttonCount = 0;             // initialize button count to start at zero

SparkFun_APDS9960 apds = SparkFun_APDS9960();


int sensorValue = 0;                      //variable for sensor reading
int ignore = 0;                           //ignore counter to ignore interrupts
//Pin Setup
const int SDchipSelect = 4;               // SD Card Chipselect
const int vib = 3;                        // Lilypad Vibrator pin

String dataString;
String cthermalstate;                    //thermal state string [Warm, Hot, Cool, Cold, Netural]
String gpsloc;

volatile int intevent = 0;               //event counter "has button been pressed"

const int ledR = A0;  //Recording
//const int ledC = A1;  //Counting
const int ledT = A3;  //Thermal State Recording

float c1=.0008236945712, c2=.0002632511198 , c3=.0000001349588752;  //Steinhart Coefficient for 103JT-025
//float c1=.001468, c2=.0002383 , c3=.0000001007; old bulky thermistor

void setup() {
// Initialize Serial Communications at 9600 bps:  
  Serial.begin(9600);

//Initialize pin2 (interrupt pin) to run interrupt routine
  pinMode(APDS9960_INT, INPUT);
  attachInterrupt(0, interruptRoutine, FALLING);
  
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
// Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
  }

  
//SD Initialization
  Serial.print(F("Initializing, SD card..."));
  pinMode(SDchipSelect, OUTPUT);
 
  if (!SD.begin(SDchipSelect)){
    Serial.println(F("Card failed, or not present"));
    return;
  }
  Serial.println("Card initialized.");
  
//String for data to log
  String dataString = "Time,int,bc, SkinTemp1,BodySensation,GPSLoc";
  
    // Check to see if the file exists: 
  if (SD.exists("datalog3.csv")) {                    //check if csv exists
    Serial.println("CSV File Exists");
    Serial.println("CSV File Deleted");            
    SD.remove("datalog3.csv");                        // delete the file:    
  }
  else {
    Serial.println("CSV File doesn't exist.");  
  }

  File dataFile=SD.open("datalog3.csv", FILE_WRITE);
  if (dataFile){
    dataFile.println(dataString);
    dataFile.close();
  }
  else{
    Serial.println("Error Openning CSV File");
  }
 
//Vibrator Setup
  
  pinMode(vib, OUTPUT);                             //pin setup for Lilypad Vibrator
  state='R';                                        //Initialize R-recording state on start up
  pinMode(A2, INPUT);                               //Setup A2 pin to read temp sensor
  pinMode(ledR, OUTPUT);                            //record placeholder   
  //pinMode(ledC, OUTPUT);                            //Count placeholder
  Serial.print("Count Interval (mS): ");
  Serial.println(countInterval);
  Serial.print("Record Interval (mS): ");
  Serial.println(recInterval);
  
  bcintstat();
  //intevent=0;                                      //Resets intevent since it gets tripped up initialization....  
} 

void interruptRoutine() {                            //When interrupted: increment event counter by 1
  intevent += 1;
}

   
void loop()  { 
  switch (state){
      case 'R':      //Data Recording State
          Serial.println("Data Recording State");
          if (intevent == 1){                         //If button was pressed
            detachInterrupt(0);
            handleGesture();  
            attachInterrupt(0, interruptRoutine, FALLING);
            vibsignal(vib,500);                      //Vibrate to ask for comfort input
            state='C';                               //Transition to button count state
            Serial.println(F("Button Was Pressed"));
            intevent=0;            //Reset event counter
            bcintstat();
            break;
          }
          else {                                    //If button wasn't pressed
            lightsignal(ledR,500);                  //Turn on/off LED indicator
            cthermalstate = "Neutral";            //Body is in neutral
            gpsloc="-";
            dataRec(2);                           //Type in # of sensors to poll, start at 
            moddelay(1000);                          //Delay X seconds before recording next data
            break;                
          }
         
      case 'X':                                  //Transition State
          intevent=0;                            //Reset intevent counter
          Serial.println(F("Transition State"));
          //lightsignal(ledC,500);
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
            handleGesture();
            vibsignal(vib,500);                                              //vibrate to indicate input 
            intevent = 0;
            bcintstat();
            state='C';
            Serial.println(F("Button pressed, returning to Button Counting State"));
            break;
          }
                  
      case 'C':                                                             //Button Counting State
          intevent = 0;                                                     //Reset event counter
          Serial.println("Button Counting State");
          //lightsignal(ledC,255);                                            // LED Indication that we are in counting state.
          //Serial.println(F("Reading button presses for 10 seconds"));
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
            buttonCount=0;
            break;
          }
            
          else if (buttonCount == -2) {                                //2-Down Swipe = Cold
            Serial.println(F("Transitioning to Thermal Recording State: Cold"));
            cthermalstate="Cold";
            state = 'T';
            intevent=0;
            bcintstat();
            buttonCount=0;
            break;
          }
              
          else if (buttonCount == 1) {                                    //1-Up Swipe = Warm
            Serial.println(F("Transitioning to Thermal Recording State: Warm"));
            cthermalstate="Warm";
            state = 'T';
            intevent=0;
            bcintstat();
            buttonCount=0;
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
        detachInterrupt(0);
        //lightsignal(ledW,255);
       //Wifi Code
        attachInterrupt(0, interruptRoutine, FALLING);                                                  //go back to low to shut off vibration                                                   //Return to button counting state
        ignore = 1;
        handleGesture();                                               //call handleGesture to reinitialize INT pin if interrupt triggered during Wifi Mode
        ignore = 0;
        vibsignal(vib,2000);                                           //vibrate to indicate input
        buttonCount = 0;                                               // reset button count
        intevent = 0;   
        state = 'C';         //vibrate signal new input
        break;
        
      case 'T':                                                        //Thermal Recording State
        detachInterrupt(0);
        Serial.println(F("Thermal Recording State"));
        lightsignal(ledT,500);
        gpsloc=String('-');
        trecording(recInterval);                // record thermal state for recording interval
        ignore = 1;
        handleGesture();                        //call handleGesture to reinitialize INT pin if interrupt triggered during Thermal State Mode
        ignore = 0;
        state = 'X';          //exit to transition state
        buttonCount = 0;                        //reset button count
        intevent = 0 ;
        attachInterrupt(0, interruptRoutine, FALLING);
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
    sensorValue = int(analogRead(A1));
    float Rt=((1024.0/sensorValue)-1)*9300.0;
    //float voltage = sensorValue *3.96/1024.0;
    //float Rt = 9943.0*((3.96/voltage)-1);
    float logRt = log(Rt);
    float T = (1.0/(c1+c2*logRt+c3*logRt*logRt*logRt))-273.15;
    float Tf = (T*9.0/5.0)+32.0;
    //sensorValue = ((sensorValue*5/1024-.5)*100)*(9/5)+32;  
    dataString += String(Tf) + ",";
    //Serial.print(F("Sensor #"));
    //Serial.print(String(i));
    //Serial.print(F("'s analog reading is "));
    //Serial.println(Tf);
  }
  dataString += cthermalstate + "," + gpsloc;        // concatenatte the thermal state and gps location
  //Serial.println(dataString);
  //Saving Data to file in SD Card
  File dataFile=SD.open("datalog3.csv", FILE_WRITE);
  if (dataFile){
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
  else{
    Serial.println(F("error opening csv"));
  }
  dataString.remove(0); 
}    

//Counter Function during Thermal Recording Mode
void trecording(int interval){
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
  //Serial.print(F("Start time: "));
  //Serial.println(startTime);
  int currentTime=millis();
  //Serial.print(F("Current time: "));
  //Serial.println(currentTime);
  int elapsedTime=(currentTime-startTime);
  //Serial.print(F("Elapsed Time mS: "));
  //Serial.println(elapsedTime);  
  while (elapsedTime < interval){      
    delay(1000);                                     //delay each iteration for 1 secs to wait for button press
    currentTime=millis();
    Serial.print(F("Elapsed Time mS: "));
    elapsedTime=(currentTime-startTime);
    Serial.println(elapsedTime);  
    //bcintstat();
            
    if (intevent > 0) {                //button was pressed 
      detachInterrupt(0);
      //Serial.println("Button was pressed");
      handleGesture();                 //determine the gesture
      attachInterrupt(0, interruptRoutine, FALLING);
      bcintstat();
      vibsignal(vib,500);
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

void handleGesture() {
    if ( apds.isGestureAvailable() ) {
    switch ( apds.readGesture() ) {
      case DIR_DOWN:      //Hotter
        if (ignore == 1)  {
          break;
        }
        else {
          buttonCount += 1;
          intevent += 1;
          vibsignal(vib,500);
          break;
        }
      case DIR_UP:                //Colder
        if (ignore == 1)  {
          break;
        }
        else {
          buttonCount += -1;
          intevent += 1;
          vibsignal(vib,500);
          break;
        }
      case DIR_RIGHT:                //WIFI
        if (ignore == 1)  {
          break;
        }
        else {
          buttonCount += 100;
          intevent += 1;
          vibsignal(vib,500);
          break;
        }
        
      case DIR_NEAR:
        if (ignore == 1)  {
          break;
        }
        else {
          buttonCount -= 20;
          intevent += 1;
          vibsignal(vib,250);
          delay(100);
          vibsignal(vib,250);
          break; 
        }
      default:
        if (ignore == 1)  {
          break;
        }
        else {
          buttonCount += 0;
          vibsignal(vib,2000);
          break;
        }
    }
  }
}
