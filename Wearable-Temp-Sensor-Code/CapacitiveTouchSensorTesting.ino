#include <CapacitiveSensor.h>

/*
 * CapitiveSense Library Demo Sketch
 * Paul Badger 2008
 * Uses a high value resistor e.g. 10M between send pin and receive pin
 * Resistor effects sensitivity, experiment with values, 50K - 50M. Larger resistor values yield larger sensor values.
 * Receive pin is the sensor pin - try different amounts of foil/metal on this pin
 */
long starttime=0;
long starttime2=0;
long stotal1, stotal2, stotal3;
long currenttime;
long duration = 5000;
int counter1 =0;
int counter2 =0;
int counter3 =0;
boolean start= 1;

//CapacitiveSensor   cs_0_1 = CapacitiveSensor(14,15);        // 1M resistor between pins 0 & 2, pin 1 is sensor pin  (down Touch Sensor)
//CapacitiveSensor   cs_2_3 = CapacitiveSensor(16,17);        // 1M resistor between pins 4 & 6, pin 3 is sensor pin  ( up Touch Sensor)
//CapacitiveSensor   cs_4_5 = CapacitiveSensor(18,19);        // 1M resistor between pins 4 & 8, pin 5 is sensor pin  (Contact/Release Sensor)



void setup()                    
{
   //cs_0_1.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   //cs_2_3.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   //cs_4_5.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example


   Serial.begin(9600);
   //pinMode(9,OUTPUT);
   //pinMode(10,OUTPUT);
   //pinMode(11,OUTPUT);
}

void loop()                    
{
    long start = millis();
    //long total1 =  cs_0_1.capacitiveSensor(30);
    //long total2 =  cs_2_3.capacitiveSensor(30);
    //long total3 =  cs_4_5.capacitiveSensor(30);
    long total1 =  touchRead(16); //  down
    long total2 =  touchRead(15);  // up
    long total3 =  touchRead(18); // pull
    
    
    
    //Serial.print(millis() - start);        // check on performance in milliseconds
                        // tab character for debug windown spacing
    Serial.print("Cap values: ");
    Serial.print(total1);                  // print sensor output 1
    Serial.print(",");
    Serial.print(total2);                  // print sensor output 2
    Serial.print(",");
    Serial.println(total3);                  // print sensor output 3
    
    if (start == 1){
      stotal1= total1;
      stotal2= total2;
      stotal3= total3;
      start = 0;
    } 
    
    Serial.print("Cap start values: ");
    Serial.print(stotal1);                  // print sensor output 1
    Serial.print(",");
    Serial.print(stotal2);                  // print sensor output 2
    Serial.print(",");
    Serial.println(stotal3);                  // print sensor output 3
    
    if(total1 > 2000 ){
      //digitalWrite(9, HIGH);
      Serial.println("Down");
      counter1 = counter1+1;
    }
    
    if(total2 > 2000){
      //digitalWrite(10, HIGH);
      Serial.println("Up");
      counter2=counter2+1;
    }
    
    if(total3 < 950){
      //digitalWrite(11, HIGH);
      Serial.println("Pull");
      counter3=counter3+1;
    }
   
    Serial.println();
    Serial.print("Counter:");
    Serial.print(counter1);
    Serial.print(",");
    Serial.print(counter2);
    Serial.print(",");
    Serial.println(counter3);
    
    if ((counter1 == 1 ||  counter2 == 1) & starttime==0) {
      starttime = millis();                  //start time at 1st button press
    }
    
    if (starttime > 0) {
      Serial.print("Time: ");
      Serial.println(millis()-starttime);
    
      if (millis()-starttime > duration){
        if (counter1 > 5 && counter2 > 5){
          starttime = 0;
          counter1 = 0;
          counter2 = 0;
          Serial.println("&^^Activated%%%%");
        }
        else{
          starttime=0;
          counter1=0;
          counter2=0;
          Serial.println(" not enough presses");  
        }
     }      
     }
     
    if (counter3 == 1 && starttime2==0) {
      starttime2 = millis();                  //start time at 1st button press
    }
      if (starttime2 > 0) {
      Serial.print("Time2: ");
      Serial.println(millis()-starttime2);
    
      if (millis()-starttime2 > duration){
        if (counter3 > 5){
          starttime2 = 0;
          counter3 = 0;
          Serial.println("&^^Pulled%%%%");
        }
        else{
          starttime2=0;
          counter3=0;
          Serial.println(" not enough pulls");  
        }
     }      
     }
     
    delay(500);
    //digitalWrite(9,LOW);  // arbitrary delay to limit data to serial port 
    //digitalWrite(10,LOW);    // arbitrary delay to limit data to serial port 
    //digitalWrite(11,LOW);    // arbitrary delay to limit data to serial port
}

