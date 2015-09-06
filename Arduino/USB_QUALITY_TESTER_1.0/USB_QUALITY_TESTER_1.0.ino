/*
                   ==========================================
                   |        USB CHARGER TESTER v1.0         |
                   ==========================================
                   |  -RAPDILY CHECKS FOR VOLTAGE CHANGES   |
                   |  -REPORTS NOISE P-P (RANGE)            |
                   |  -10mV Accuracy                        |
                   |  -WORKS WITH SERIAL AND 16X2 LCD       |
                   |  -TESTS CHARGERS WITH AND WITHOUT LOAD |
                   |  -TEMPERATURE TESTING AND ALARM        |
                   |  -GOOD AND BAD LED's                   |
                   |  -AUDIBLE TONES VIA PEIZO              |
                   |  -BATTERY MONITORING AND PWR SAVING    |
                   |  -HIGHLY CUSTOMISABLE WITH VARIABLES   |
                   |  -CAN BE CALIBRATED (to around 10mV)   |
          ============================================================

                         Sketch/schematic by Ben Duffy,
           http://microsoldering.com.au, geelongmicrosoldering@gmail.com
-----------------------------------------------------------------------------------
               This code is in developement. It is likely to have bugs.
 It is not optimised, and a web of horrible coding ideas, but its better than it was.
                 I eventually want to be able to measure current too.
-----------------------------------------------------------------------------------
Minor Bugs:
----------- 
-Some kind of loop bug exists right now, so starting via serial (at the press a key prompt via serial monitor)
will cause an infinite loop. Use the Start Button instead (connect startpin to gnd momentarily)

-A bug exists that randomly causes test not to start after periods of inactivity
(everything else happens as expected when button is pressed but test does not start. Reset fixes)

-The time between power save reminders takes longer than expected

TO DO!
------
-measure current by measuring voltage on return of load

-lcd final report output is not finished

-Need to finish temp monitor code at end of loop. does not report min/max temp to user in final report
otherwise warnings and alarms are working.

-Optimise the code in many ways. Low priority. Ill do it when i have no memory left or have to for some reason
-----------------------------------------------------------------------------------

VOLTADE DIVIDER NOTE:
Any resistors over 1k will work, as long as they are all the same, however:
-The resistors must be as accurate as possible (5% is not good enough)
-A value of 10mohm or more would be prefferred as it is less likely to interfere with the circuit being tested.
-Higher values will however pick up more residual noise, so build cicuits well, and do not touch it while it is in use.
-Many chargers have 1-100k resistors inside them on the data pins, as a voltage divider.
-Connecting lower value resistors will have an adverse effect on data pin voltage.
-The data pin voltage will almost always be inaccurate anyway, due to internal resistors in most chargers.


*/


 //include the library for LCD
#include <LiquidCrystal.h>
//Define pins for LCD (leave this unless you have wired the lcd differently)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

  

//=================================================================================
//YOU CAN CUSTOMISE THE VALUES BELOW
//=================================================================================


//MAIN SETTINGS
int batterymode = 1;         //Enable battery management mode
int bootbatt = 1;         //Show battery percentage on boot. ON (1) or OFF(0
int lcdmode = 1 ;         //Turn the lcd ON (1) or OFF(0)
int serialmode = 0;          //Turn the serial output ON(1) or OFF(0)
int serialverbose = 0;          //Turn verbose serial output ON(1) or OFF(0)
int beepon = 1;         //Turn the audible beep ON (1) or OFF(0)
int loadtest = 1;         //Turn testing with load ON (1) or OFF(0)
int temptest = 0;         //Turn temperature testing ON (1) or OFF(0)
int runmode = 0;          //The default setting for run mode at boot (0=Wait for start button, 1=start test immediately)
int backlightlevel = 90;         //The level of lcd backlight (0-100%, lower to save power)
int backlightbattlevel = 10;         //The level of lcd backlight in battery mode (0-100%, lower to save power)

//TIMING SETTINGS
long cycledelay = 3000;         //The delay after each test when results are on screen
long powersave = 8000;         //The delay of inactivity before enetering power saving mode
long powersaveremind = 30000;         //The delay of time after entering power saver mode before reminding the user the device is on
long lowbattint = 15000;         //The delay between low battery warnings
int reportinterval = 500;         //The interval to report to user (Leave this unless you have a good reason to change it)
int avginterval = 50;         //The interval to average collected data (Leave this unless you have a good reason to change it)
int voltbaddelay = 2000;         //The delay the too high/too low voltage message appears on the lcd     
int cycle2enddelay = 5000;        //The delay after testing has finished, before turning the relay off (this will make your tests take longer, but will help in detecting a charger that overheats after x amount of time) 
int tempdelay = 1000;         //The delay between temp tests/warning (does not effect alarm which has no delay)

//PIN ASSIGNMENT
int startpin = A0;         //The pin the start button connects to (Leave this unless you have a good reason to change it)
int loadpin = A4;         //The pin the load relay connects to (Leave this unless you have a good reason to change it)
int temppin = A6;         //The pin the temp sensor connects to (Leave this unless you have a good reason to change it)
int goodledpin = 12;         //The pin the GOOD LED connects to (Leave this unless you have a good reason to change it)
int badledpin = 11;         //The pin the BAD LED connects to (Leave this unless you have a good reason to change it)
int beeppin = 3;         //The pin the peizo speaker connects to (Leave this unless you have a good reason to change it)
int backlightpin = 10;         //The pint the backlight Cathode (-) connects to (used to turn backlight on and off)
//THE FOLLOWING PINS ARE FOR USB CHARGER INPUT. VOLTAGE DIVIDERS ARE USED ON THESE PINS (100k to signal and 100k to gnd)
int USB5V = A1;         //The voltage divider pin for USB5V+
int USBDN = A2;         //The voltage divider pin for USB DATA-
int USBDP = A3;         //The voltage divider pin for USB DATA+
//THE FOLLOWING PIN IS FOR BATTERY LEVEL INPUT. A VOLTAGE DIVIDER MUST BE USED. (100k to signal and 100k to gnd)
int battpin = A5;         //The voltage divider pin that connects to the battery/VIN




//ACCEPTABLE/UNACCEPTABLE READINGS
int warningtemp = 60;         //The temperature at which to warn of possible overheating (celsius)
int alarmtemp = 70;         //The temperature at which to alarm and end testing (celsius)

int horribleresult = 150;         //The minimum level of noise p-p (mV) considered beyond bad/horrible
int badresult = 95;         //The maximum level of noise p-p (mV) that will be accepted
int acceptableresult = 60;         //The minimum level of noise p-p (mV) considered suspicious          __A "good" result is between these two values
int greatresult = 40;         //The maximum level of noise considered great/fantastic/ideal              /

int okhighmv = 5300;         //Voltages above this (mV) are considered suspicious
int oklowmv = 4850;         //Voltage below this (mV) are considered suspicious
int highmv = 5450;         //The maximum voltage reading (mV) accepted
int lowmv = 4700;         //The minimum voltage reading (mV) accepted
int allowedvoltdrop = 400;         //Voltage drops above this (mV) are considered bad.

float fullbattreading = 9000;         //The battery voltage (mV) considered fully charged
float lowbattreading = 6500;         //The battery voltage (mV) considered low
float flatbattreading = 6000;         //The battery voltage (mV) considered flat


//ADVANCED SETTINGS
//To calibrate, first set the int1V1Ref to 1125300L
//write down the VCC listed in the final report via serial, or via the LCD in calibrate mode, then take a reading of the 5V pin with an accurate meter.
//The "real1V1" value in the example below is calculated as "1.1 * Meter Reading / VCC". 
long int1V1Ref = 1089105L;         // There should also be an "L" at the end. eg: 1125300L (1125300 = real1V1 * 1023 * 1000;)
int calibrateon = 0;         //Turn calibrate mode ON (1) or OFF (0) (This will constantly display VCC reading with a 1 second delay on the LCD


//You can change the below setting depending on memory requirements, accuracy ect. 150 is plenty. you can use less if you want faster tests, or more if you want better accuracy.
const int NoiseNumReadings = 150;         //The number of noise readings to take per pass (there will be 2 passes, so double will occur in total.)

//=================================================================================
//END OF USER DEFINED VARIABLES. LEAVE THE REST OF THE CODE ALONE
//=================================================================================






















byte lb1[8] = {
  0b11111,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b11111
};

byte lb2[8] = {
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111
};

byte lb3[8] = {
  0b11111,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b00001,
  0b11111
};

byte lb4[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

byte lowbat[8] = {
  0b01110,
  0b11011,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
  0b11111
};

int currenttemp;
int TEMPMIN;
int TEMPMAX;
int tempstarttime = millis();
int lowbatstarttime = millis();
int lowbatcurrenttime;
int tempcurrenttime;
int backlightlvl;
int powerstarttime = millis();
int powercurrenttime;
int powersaveon;
int powerremindcurrenttime;
int powerremindstarttime;
int starttime = millis();
int avgstarttime = millis();
int currenttime;
int USB5VMIN;
int USB5VMAX;
int USB5VMIN2;
int USB5VMAX2;
int USBDPVALUE;
int USBDNVALUE;
int USBDPVALUE2;
int USBDNVALUE2;
int USB5VVALUE;
int USB5VVALUE2;
int USB5VVOLTAGE;
int USB5VVOLTAGE2;
int USBDPVOLTAGE;
int USBDNVOLTAGE;
int USBDPVOLTAGE2;
int USBDNVOLTAGE2;
int voltagedrop;
int NOISEMIN;
int NOISEMAX;
int NOISEMIN2;
int NOISEMAX2;
int TOTNOISEMIN;
int TOTNOISEMAX;
int currentnoise;
int cheatboot=0;
int serialdata;
char* x1;
char* x2;
double vcc;
int BATTVALUE;
int BATTVOLTAGE;
int BATTPERCENT;
int battwarning;
int battempty;
int relayon;

int NoiseReadings[NoiseNumReadings];      // the readings from the analog input
int noiseIndex = 0;                  // the index of the current reading
int noiseTotal = 0;                  // the running total
int noiseAverage = 0;                
int noiseAverage2 = 0;               
int alreadyprompted = 0;
int progress = 0;
int starting;




void setup() {
digitalWrite(backlightpin, LOW);
if (batterymode == 1){
backlightlvl = (255 / 100) * backlightbattlevel;
} else {backlightlvl = (255 / 100) * backlightlevel;}
analogWrite(backlightpin, backlightlvl);
pinMode(backlightpin, OUTPUT);
analogReference(INTERNAL);
pinMode(startpin, INPUT_PULLUP);
pinMode(loadpin, OUTPUT);
pinMode(goodledpin, OUTPUT);
pinMode(badledpin, OUTPUT);
pinMode(beeppin, OUTPUT);
digitalWrite(loadpin, HIGH);
led('G', HIGH);
led('B', HIGH);

beep(20, 50, 1);

if (serialmode == 1) {
 Serial.begin(9600);}
if (lcdmode == 1) {
lcd.createChar(1, lb1);
lcd.createChar(2, lb2);
lcd.createChar(3, lb3);
lcd.createChar(4, lb4);
lcd.createChar(5, lowbat);
 lcd.begin(16, 2);}


 
  for (int thisNoiseReading = 0; thisNoiseReading < NoiseNumReadings; thisNoiseReading++) {
    NoiseReadings[thisNoiseReading] = 0; 
    }

if (cheatboot == 0) {
//HELP MESSAGE AT BOOTUP
char* boothelpmsg = " FOR HELP EMAIL";
char* bootcontact = "geelongmicrosoldering@gmail.com";

//BOOT DISPLAY (SERIAL)    
    if (serialmode == 1) {
    Serial.println();
    Serial.println();
    Serial.println(F("================================================================================="));
    Serial.println(F("==                        USB CHARGER  QUALITY TESTER    v1.0                  =="));
    Serial.println(F("================================================================================="));
    Serial.println(F("                                            by Ben Duffy, Geelong Micro Soldering"));
    Serial.println();
    Serial.println();
}

//BOOT DISPLAY (LCD)    
    if (lcdmode == 1) {
    lcdpr("  USB  CHARGER  ", " QUALITY TESTER",1500);
    lcdpr("   by Geelong", "Micro  Soldering",1000);
    size_t bootcont = strlen(bootcontact);
    int scroller = 1;
    lcdpr(boothelpmsg, bootcontact,1000);
    if(bootcont > 16) { 
    int bootcontlength = bootcont-16;
    for (int positionCounter = 0; positionCounter < bootcontlength; positionCounter++) {
    lcd.scrollDisplayLeft();
    lcd.setCursor(scroller++, 0);
    lcd.print(boothelpmsg);
    delay(200);}
    delay(500);
  } else { delay(1500); }
lcd.clear();    
    
}}
//READ BATTERY PERCENTAGE AT BOOT

 vcc = readVcc();
 BATTVALUE = analogRead(battpin);
 BATTVOLTAGE = (BATTVALUE / 1023.0) * vcc * 2;
 
BATTPERCENT = ((BATTVOLTAGE -flatbattreading) / (fullbattreading-flatbattreading)) * 100;

if (bootbatt == 1) {
if (lcdmode == 1) {
lcd.clear();
lcd.print(" Battery Level:");
lcd.setCursor(0,1);
lcd.print("   ");
lcd.print(BATTVOLTAGE);
lcd.print("mV  ");
lcd.print(BATTPERCENT);
lcd.print("%");
delay(2000);
}

if (serialmode == 1) {
  Serial.print(F(" Battery Level:  "));
  Serial.print(BATTVOLTAGE);
  Serial.print(F("(mV)  "));
  Serial.print(BATTPERCENT);
  Serial.println(F("%"));
  
}





}

if (runmode == 1){
  alreadyprompted=1;}

}





void loop() {


//TURN CALIBRATE MODE ON IF APPLICABLE
if (calibrateon == 1) {
  if(int1V1Ref == 1125300L) {
  lcd.clear();
  lcd.print(" CALIBRATE MODE");
  lcd.setCursor(0,1);
  lcd.print("VCC:      ");
  lcd.print(readVcc());
  lcd.print("mV");
  delay(1000);
  } else {
  lcd.clear();
  lcd.print(" CALIBRATED!!!!");
  lcd.setCursor(0,1);
  lcd.print(readVcc());
  lcd.print("mV ");
  lcd.print(int1V1Ref);
  lcd.print("L");
  delay(1000);
  
    }
  }



//CHECK TO SEE IF BATTERY IS COMPLETELY FLAT
if (batterymode == 1){
  if (battwarning == 1){} else {
 
 vcc = readVcc();
 BATTVALUE = analogRead(battpin);
 BATTVOLTAGE = (BATTVALUE / 1023.0) * vcc * 2;
 
if (BATTVOLTAGE < flatbattreading) {
battwarning = 1;
runmode = 0;
alreadyprompted = 1;
battempty = 1;
digitalWrite(badledpin, HIGH);

if (lcdmode == 1) {
lcd.clear();
  lcd.print("  FLAT BATTERY ");
  lcd.write(byte(5));
  lcd.setCursor(0, 1);
  lcd.print("CANNOT CONTINUE!");

}

if (serialmode == 1) {
  Serial.println(F("ERROR: FLAT BATTERY!! CANNOT CONTINUE"));
  }

}}}


//IF NOT, CHECK IF BATTERY LEVEL IS LOW
if (batterymode == 1){
if (battwarning == 1){
  lowbatcurrenttime = millis();
  if (lowbatcurrenttime-lowbatstarttime > lowbattint){
  lowbatstarttime = millis();
if (lcdmode == 1) {
  analogWrite(backlightpin, backlightlvl); 
lcd.clear();
  lcd.print("  LOW  BATTERY ");
  lcd.write(byte(5));
  lcd.setCursor(0, 1);
  lcd.print("  REPLACE SOON");
}
if (serialmode == 1) {
  Serial.println(F("WARNING: LOW BATTERY. REPLACE SOON"));
  }
  
  
  }
  } else {
 vcc = readVcc();
 BATTVALUE = analogRead(battpin);
 BATTVOLTAGE = (BATTVALUE / 1023.0) * vcc * 2;
 
if (BATTVOLTAGE < lowbattreading) {
battwarning = 1;
if (lcdmode == 1) {
lcd.clear();
  lcd.print("  LOW  BATTERY ");
  lcd.write(byte(5));
  lcd.setCursor(0, 1);
  lcd.print("  REPLACE SOON");
}
if (serialmode == 1) {
  Serial.println(F("WARNING: LOW BATTERY. REPLACE SOON"));
  }
  
  digitalWrite(badledpin, LOW);
  delay(250);
  digitalWrite(badledpin, HIGH);
  delay(250);
  digitalWrite(badledpin, LOW);
  delay(250);
  digitalWrite(badledpin, HIGH);
  delay(250);
  digitalWrite(badledpin, LOW);
  delay(250);
  digitalWrite(badledpin, HIGH);
  delay(250);
  digitalWrite(badledpin, LOW);
  delay(250);
  digitalWrite(badledpin, HIGH);

}
}
}


if (runmode == 1) {
analogWrite(backlightpin, backlightlvl); 

//TEST WITHOUT LOAD  
 
 vcc = readVcc();
 USB5VVALUE = analogRead(USB5V);
 USB5VVOLTAGE = (USB5VVALUE / 1023.0) * vcc * 2;

 if (USB5VMIN == 0 || USB5VMAX == 0) {
  USB5VMIN = USB5VVOLTAGE;
  USB5VMAX = USB5VVOLTAGE;
  } else {
currentnoise = USB5VMAX-USB5VMIN;
  }
if (USB5VVOLTAGE > USB5VMAX) {USB5VMAX = USB5VVOLTAGE;}
if (USB5VVOLTAGE < USB5VMIN) {USB5VMIN = USB5VVOLTAGE;}

if (NOISEMIN == 0) {NOISEMIN = currentnoise;}
 if (NOISEMAX == 0) {NOISEMAX = currentnoise;}

if (currentnoise > NOISEMAX) {NOISEMAX = currentnoise;}
if (currentnoise < NOISEMIN) {NOISEMIN = currentnoise;}

 
 currenttime = millis();
if (currenttime-starttime > reportinterval) 
  { 
    if (serialmode == 1) {
    Serial.print(F("TESTING (NO LOAD). PLEASE WAIT... "));
    if (serialverbose == 1) {      
    Serial.print(F("(    VCC: "));
    Serial.print(USB5VMIN);
    Serial.print(F("-"));
    Serial.print(USB5VMAX);
    Serial.print(F("mV"));
    Serial.print(F("     NOISE: "));
    Serial.print(currentnoise);
    Serial.print(F("mV, RANGE: "));
    Serial.print(NOISEMIN);
    Serial.print(F("-"));
    Serial.print(NOISEMAX);
    Serial.print(F("    )"));
    }
    Serial.println();
    }
    loadbar("TESTING...");



    powerstarttime = currenttime;
    starttime = currenttime;
    USB5VMIN=0;
    USB5VMAX=0;

  }

if (currenttime-avgstarttime > avginterval) { 
     // subtract the last reading:
  noiseTotal= noiseTotal - NoiseReadings[noiseIndex];        
  // read from the sensor:  
  NoiseReadings[noiseIndex] = currentnoise;
  // add the reading to the total:
  noiseTotal= noiseTotal + NoiseReadings[noiseIndex];      
  // advance to the next position in the array:  
  noiseIndex = noiseIndex + 1;                    
  
  progress = progress + 1;
  // if we're at the end of the array...
  if (noiseIndex >= NoiseNumReadings) {             
    // ...wrap around to the beginning:
    



   vcc = readVcc();
 
   USBDPVALUE = analogRead(USBDP);
   USBDPVOLTAGE = (USBDPVALUE / 1023.0) * vcc * 2;
   USBDNVALUE = analogRead(USBDN);
   USBDNVOLTAGE = (USBDPVALUE / 1023.0) * vcc * 2;

   if (loadtest == 1) {
   runmode = 2;
   } else {runmode = 3;}
   
   noiseIndex = 0;   
  }
  
    // calculate the average:
  noiseAverage = noiseTotal / NoiseNumReadings;

avgstarttime = currenttime;
relayon = 1;
}
}




if (runmode == 2) {
   analogWrite(backlightpin, backlightlvl); 
//TESTING WITH LOAD  
if (relayon == 1) {
 digitalWrite(loadpin, LOW);
 delay(1000);
 relayon = 0;
}
 
 vcc = readVcc();
 USB5VVALUE2 = analogRead(USB5V);
 USB5VVOLTAGE2 = (USB5VVALUE2 / 1023.0) * vcc * 2;

 if (USB5VMIN2 == 0 || USB5VMAX2 == 0) {
  USB5VMIN2 = USB5VVOLTAGE2;
  USB5VMAX2 = USB5VVOLTAGE2;
  } else {
currentnoise = USB5VMAX2-USB5VMIN2;
  }
if (USB5VVOLTAGE2 > USB5VMAX2) {USB5VMAX2 = USB5VVOLTAGE2;}
if (USB5VVOLTAGE2 < USB5VMIN2) {USB5VMIN2 = USB5VVOLTAGE2;}

if (NOISEMIN2 == 0) {NOISEMIN2 = currentnoise;}
 if (NOISEMAX2 == 0) {NOISEMAX2 = currentnoise;}

if (currentnoise > NOISEMAX2) {NOISEMAX2 = currentnoise;}
if (currentnoise < NOISEMIN2) {NOISEMIN2 = currentnoise;}

 
 currenttime = millis();
if (currenttime-starttime > reportinterval) 
  { 
    if (serialmode == 1){
    Serial.print(F("TESTING (WITH LOAD). PLEASE WAIT... "));
    if (serialverbose == 1) {      
    Serial.print(F("(    VCC: "));
    Serial.print(USB5VMIN2);
    Serial.print(F("-"));
    Serial.print(USB5VMAX2);
    Serial.print(F("mV"));
    Serial.print(F("     NOISE: "));
    Serial.print(currentnoise);
    Serial.print(F("mV, RANGE: "));
    Serial.print(NOISEMIN2);
    Serial.print(F("-"));
    Serial.print(NOISEMAX2);
    Serial.print(F("    )"));
    }
    Serial.println();
    }
    loadbar("TESTING...(LOAD)");


    powerstarttime = currenttime;
    starttime = currenttime;
    USB5VMIN2=0;
    USB5VMAX2=0;

  }

if (currenttime-avgstarttime > avginterval) { 
     // subtract the last reading:
  noiseTotal= noiseTotal - NoiseReadings[noiseIndex];        
  // read from the sensor:  
  NoiseReadings[noiseIndex] = currentnoise;
  // add the reading to the total:
  noiseTotal= noiseTotal + NoiseReadings[noiseIndex];      
  // advance to the next position in the array:  
  noiseIndex = noiseIndex + 1;                    
  
  progress = progress + 1;
  // if we're at the end of the array...
  if (noiseIndex >= NoiseNumReadings) {             
    // ...wrap around to the beginning:
    



   vcc = readVcc();
 
   USBDPVALUE2 = analogRead(USBDP);
   USBDPVOLTAGE2 = (USBDPVALUE2 / 1023.0) * vcc * 2;
   USBDNVALUE = analogRead(USBDN);
   USBDNVOLTAGE2 = (USBDPVALUE2 / 1023.0) * vcc * 2;
   if (cycle2enddelay > 1) {
    loadbar("   WAITING...");
    delay(cycle2enddelay);
  }
  digitalWrite(loadpin, HIGH);
   runmode = 3;
   noiseIndex = 0;
  }
  
    // calculate the average:
  noiseAverage2 = noiseTotal / NoiseNumReadings;

  avgstarttime = currenttime;
}   
}



    //FINAL REPORT
if (runmode == 3) {

//TO DO:
//CHECK TEMP MAX AND SEE IF IT EXCEEDS WARNING OR ALARM TEMP, AND REPORT IT EITHER WAY
  
  analogWrite(backlightpin, backlightlvl); 
  progress = 0;
  powerstarttime = millis();
    if (serialmode == 1){
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println(F("================================================================================="));
    Serial.println(F("==                                  FINAL REPORT                               =="));
    Serial.println(F("================================================================================="));
    Serial.print(F("                                                    VCC:"));
    Serial.print(readVcc());
    Serial.print(F("(mV) BATT: "));
    Serial.print(BATTVOLTAGE);
    Serial.print("(mV) ");
    Serial.print(BATTPERCENT);
    Serial.println("%");
    Serial.println(F("                                         NO LOAD"));
    Serial.println(F("                                   =================="));
    Serial.print(F("                            USB5V: "));
    Serial.print(USB5VMIN);
    Serial.print(F(" - "));
    Serial.print(USB5VMAX);
    Serial.println(F(" mV"));
    Serial.print(F("                            DATA+: "));
    Serial.print(USBDPVOLTAGE);
    Serial.println(F(" mV"));
    Serial.print(F("                            DATA-: "));
    Serial.print(USBDNVOLTAGE);
    Serial.println(F(" mV"));
    Serial.print(F("                    AVERAGE NOISE: "));
    Serial.print(noiseAverage);
    Serial.println(F(" mV"));
    Serial.print(F("                        NOISE P-P: "));
    Serial.print(NOISEMIN);
    Serial.print(F(" - "));
    Serial.print(NOISEMAX);                       
    Serial.println(F(" mV (MIN - MAX)"));
    Serial.println();
if (loadtest == 1) {   
    Serial.println(F("                                        WITH LOAD"));
    Serial.println(F("                                   =================="));
    Serial.print(F("                            USB5V: "));
    Serial.print(USB5VMIN2);
    Serial.print(F(" - "));
    Serial.print(USB5VMAX2);
    Serial.println(F(" mV"));
    Serial.print(F("                            DATA+: "));
    Serial.print(USBDPVOLTAGE2);
    Serial.println(F(" mV"));
    Serial.print(F("                            DATA-: "));
    Serial.print(USBDNVOLTAGE2);
    Serial.println(F(" mV"));
    Serial.print(F("                    AVERAGE NOISE: "));
    Serial.print(noiseAverage2);
    Serial.println(F(" mV"));
    Serial.print(F("                        NOISE P-P: "));
    Serial.print(NOISEMIN2);
    Serial.print(F(" - "));
    Serial.print(NOISEMAX2);                       
    Serial.println(F(" mV (MIN - MAX)"));
    Serial.print(F("                      VOLTAGE DROP:"));
    Serial.print(voltagedrop);
    Serial.println(F("mV (WITH LOAD)"));
    Serial.println();
    }    
    
    Serial.print(F(" NOISE LEVEL: "));
    }
 
 //NOISE RESULT OUTPUT. MAKING ALL OF THESE INTO A FUNCTION WOULD HAVE BEEN CLEANER AND SAVED PROGRAM SPACE, BUT THERE PROBABLY ISN'T ENOUGH MEMORY FOR VARIABLES
 if (NOISEMAX > horribleresult || NOISEMAX2 > horribleresult) {
    
    Serial.println(F("WAY TOO MUCH NOISE!! THROW IT IN THE BIN!!"));
    if (lcdmode == 1) {
    lcdpr("   HORRIBLE!!", "THROW IT AWAY!!!",0);
    }
    led('B', LOW);
    x2="X";
    beep(20, 500, 4);
    } else {
 if (NOISEMAX > badresult || NOISEMAX > badresult) {
    Serial.println(F("TOO MUCH NOISE!! BAD CHARGER"));
    if (lcdmode == 1) {
    lcdpr("TOO MUCH NOISE!!", "  BAD  CHARGER",0);
    }
    led('B', LOW);
    x2="X";
    beep(20, 500, 4);
    } else {
 if (NOISEMAX > acceptableresult || NOISEMAX > acceptableresult) {
    Serial.println(F("SUSPICIOUS, BUT ACCEPTABLE"));

    if (lcdmode == 1) {
    lcdpr(" SUSPICIOUS BUT", "   ACCEPTABLE",0);
    }
    led('B', LOW);
    led('G', LOW);
    beep(20, 500, 1);
    beep(20, 100, 1);
    beep(20, 500, 1);
    beep(20, 100, 1);
    x2="?";
    } else {
if (NOISEMAX < greatresult || NOISEMAX < greatresult) {
    Serial.println(F("  FANTASTIC!!"));
    if (lcdmode == 1) {
    lcdpr(" FANTASTIC!!", " GREAT  QUALITY",0);
    }
    led('G', LOW);
    beep(20, 33, 4);
    } else {
    Serial.println(F("GOOD!! TEST PASSED"));
if (lcdmode == 1) {
    lcdpr("     GOOD", "  TEST  PASSED",0);
    }
    led('G', LOW);
    beep(20, 60, 4);
  }
  }
  }
  }



if (USB5VMAX > okhighmv || USB5VMAX2 > okhighmv) {
    x1="?";
    }

if (USB5VMIN < oklowmv || USB5VMIN2 < oklowmv) {
    x1="?";
    }

Serial.println();

if (USB5VMAX > highmv || USB5VMAX2 > highmv) {
    Serial.println(F("              >>   VOLTAGE TOO HIGH!!!   <<"));
    if (lcdmode == 1) {
    lcdpr("VOLTAGE HIGH!!!!", "  BAD  CHARGER",0);
    }
    led('B', LOW);
    led('G', HIGH);
    x1="X";
    beep(20, 500, 4);
    delay(voltbaddelay);
    }


if (USB5VMIN < lowmv || USB5VMIN2 < lowmv) {
    Serial.println(F("              >>   VOLTAGE TOO LOW!!!   <<"));
    if (lcdmode == 1) {
    lcdpr("VOLTAGE  LOW!!!!", "  BAD  CHARGER",0);
    }
    led('B', LOW);
    led('G', HIGH);
    x1="X";
    beep(20, 500, 4);
    delay(voltbaddelay);
    }

voltagedrop = (USB5VMIN + USB5VMAX / 2) - (USB5VMIN2 + USB5VMAX2 / 2);

if (voltagedrop > allowedvoltdrop) {
    Serial.println(F("         >>   VOLTAGE DROPS TOO MUCH!!!   <<"));
    Serial.print(F("                 ("));
    Serial.print(allowedvoltdrop);
    Serial.println(F(" mV allowed)"));
    
    if (lcdmode == 1) {
    lcdpr("VOLTAGE  DROP!!!", "  BAD  CHARGER",0);
    }
    led('B', LOW);
    led('G', HIGH);
    x1="X";
    beep(20, 500, 4);
    delay(voltbaddelay);
    }

  Serial.println(F("================================================================================="));
    
  if (lcdmode == 1) {
    delay(1000);
    lcd.clear();
    lcd.print(USB5VMIN/1000.00);
    lcd.print("-");
    lcd.print(USB5VMAX/1000.00);
    lcd.print("V (v");
    lcd.print(voltagedrop);
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("P-P:");
    lcd.print(NOISEMIN);
    lcd.print("-");
    lcd.print(NOISEMAX);
    lcd.print("mV");
    lcd.setCursor(14, 0);
    lcd.print(x1);
    if (battwarning == 1){lcd.write(byte(5));}
    lcd.setCursor(14, 1);
    lcd.print(x2);
    beep(20, 30, 1);
    delay(80);
    beep(20, 30, 1);
    delay(3000);
    }




//END OF TEST, STARTING AGAIN
USB5VVALUE = 0;
USB5VVOLTAGE = 0;
USB5VMIN = 0;
USB5VMAX = 0;
USB5VMIN2 = 0;
USB5VMAX2 = 0;
USBDPVALUE = 0;
USBDNVALUE = 0;
USBDPVOLTAGE = 0;
USBDNVOLTAGE = 0;
NOISEMIN = 0;
NOISEMAX = 0;
TOTNOISEMIN = 0;
TOTNOISEMAX = 0;
currentnoise = 0;
serialdata = 0;
x1 = "";
x2 = "";
    alreadyprompted = 0;
    runmode = 0;
delay(cycledelay);
  } else {




    
//READY TO START  
if (alreadyprompted == 0) {
    Serial.println();
    Serial.println(F("READY TO TEST. PLEASE PRESS THE START BUTTON, OR TYPE A KEY AND PRESS ENTER"));

if (lcdmode == 1) {
    lcdpr(" READY FOR TEST", "  PRESS  START",0);
    if (battwarning == 1){
      lcd.setCursor(15, 0);
      lcd.write(byte(5));
      }
    
}    

beep(20, 50, 2);
alreadyprompted = 1;
if (batterymode == 1) {powerstarttime = millis();}

}

//ENTER POWER SAVE MODE
if (batterymode == 1) {
powercurrenttime = millis();
if (powercurrenttime-powerstarttime > powersave) {
  analogWrite(backlightpin, 0); 
  powersaveon = 1;
  powerremindstarttime = millis();
  }


if (powersaveon == 1){
powerremindcurrenttime = millis();
if (powerremindcurrenttime-powerremindstarttime > powersaveremind) {
analogWrite(backlightpin, backlightlvl); 
lcdpr("   REMINDER!!","DEVICE STILL ON!",0);
    if (battwarning == 1){
      lcd.setCursor(15, 0);
      lcd.write(byte(5));
      }
      
beep(20, 500, 1);
beep(20, 500, 1);
beep(20, 500, 1);
delay(3000);
analogWrite(backlightpin, 0); 
powerremindstarttime = millis();
alreadyprompted = 0;  
}
}



}

if (serialmode == 1) {serialdata = Serial.available();}

//WHEN START BUTTON IS PRESSED

if (digitalRead(startpin) == LOW || serialdata > 1) {
serialdata = 0;

//IF BATTERY EMPTY BEEP AND DO NOTHING
if (battempty == 1) {
  analogWrite(backlightpin, backlightlvl); 
  beep(20, 1000, 1);
  powerremindstarttime = millis();
  

} else {
  //IF BATTERY OK CONTINUE
digitalWrite(badledpin, HIGH);
digitalWrite(goodledpin, HIGH);
analogWrite(backlightpin, backlightlvl); 
powerstarttime = millis();
powerremindstarttime = millis();
runmode = 1;
beep(20, 100, 1);
  }    




}
}


//ALWAYS CHECK TEMP IF TEMP TESTING IS ON - MOVE THIS TO END OF LOOP
if (temptest == 1) {

//TO DO:

//report min and max temp in final report

tempcurrenttime = millis();

if (tempstarttime-tempcurrenttime > tempdelay) {
  tempstarttime = millis();

vcc = readVcc();
 int TEMPVALUE = analogRead(temppin);
 currenttemp = ((TEMPVALUE / 1023.0) * vcc) / 10;
//set min and max temp

 if (TEMPMIN == 0 || TEMPMAX == 0) {
  TEMPMIN = currenttemp;
  TEMPMAX = currenttemp;
  }
if (currenttemp > TEMPMAX) {TEMPMAX = currenttemp;}
if (currenttemp < TEMPMIN) {TEMPMIN = currenttemp;}
  

if (currenttemp >= warningtemp){
if (lcdmode == 1) {
lcd.clear();
lcd.print(" TEMP WARNING!!");
lcd.setCursor(0,1);
lcd.print("      ");
lcd.print(currenttemp);
lcd.print(" C");
}
if (serialmode == 1) {
  Serial.print(F(" TEMP WARNING:  "));
  Serial.print(currenttemp);
  Serial.println(F(" (C)"));
}
digitalWrite(badledpin, LOW);
delay(2);
digitalWrite(badledpin, HIGH);
}

  
  }



if (currenttemp >= alarmtemp){
if (lcdmode == 1) {
lcd.clear();
lcd.print(" TEMP ALARM!!");
lcd.setCursor(0,1);
lcd.print("      ");
lcd.print(currenttemp);
lcd.print(" C");
}
if (serialmode == 1) {
  Serial.print(F(" TEMP ALARM!! :  "));
  Serial.print(currenttemp);
  Serial.println(F(" (C)"));
}
digitalWrite(loadpin, HIGH);
digitalWrite(badledpin, LOW);
runmode = 0;
beep(20, 350, 3);


} 


  
  }


}
void beep(unsigned char frequency, unsigned char delayms, int repeat){

if (beepon == 1) {  
  for (int x = 0; x < repeat; x++) {
  analogWrite(beeppin, frequency);
  delay(delayms);
  analogWrite(beeppin, 0);     
  delay(delayms);          
  }
}

}  


void led(unsigned char ledid, unsigned char ledmode){
  if (ledid == 'G') { digitalWrite(goodledpin, ledmode);}
  if (ledid == 'B') { digitalWrite(badledpin, ledmode);}

} 


void lcdpr(char* line1, char* line2, int unsigned lcddelay){
if (lcdmode == 1) {
    lcd.clear();
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    delay(lcddelay);}
}


void loadbar(char* loadtext){
if (lcdmode == 1) {

  int prog; 
  
  if (loadtest == 1) {
  prog = progress / (NoiseNumReadings/8.5);
  } else { prog = progress / (NoiseNumReadings/17);}

  
  lcd.clear();
  lcd.print(loadtext);
  lcd.setCursor(0, 1);
  lcd.write(byte(1));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.setCursor(0, 1);
for (int i=0; i < prog; i++){
  lcd.write(byte(4));
}

  }
}   



long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(1); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;

  
  result = int1V1Ref / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
