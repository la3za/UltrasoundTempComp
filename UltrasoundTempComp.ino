/*
 Ultrasound distance measurement which is compensated for temperature variation
 Ultrasound sensor : HC-SR04
 Temperature sensor: DS1820
 LCD:                2 x 16 lines
Created by merging code from
     http://www.tautvidas.com/blog/2012/08/distance-sensing-with-ultrasonic-sensor-and-arduino/
 and http://playground.arduino.cc/Learning/OneWire (Last program on this page)
 
  Sverre Holm, 30 May 2014
  la3za (a) nrrl.no
*/

#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

#define FIXEDSPEED 0 // turn off temp compensation if == 1

/* Ultrasound sensor */
int pingPin = 12; 
int inPin = 13; 

/* DS18S20 Temperature chip i/o */

OneWire ds(24); // (4.7K to Vcc is required)

#define MAX_DS1820_SENSORS 1
byte addr[MAX_DS1820_SENSORS][8];

int RepeatTemp = 100; // Temp measurement is done every 100*0.1 sec = 10 sec



void setup() { 
  
  lcd.begin(LCD_WIDTH, LCD_HEIGHT); 
  lcd.print("init ..."); 
  
  //delay(1000);

   if (!ds.search(addr[0])) 
   {
     lcd.setCursor(0,0);
     lcd.print("No more addr");
     ds.reset_search();
     delay(250);
     return;
   }
   if ( !ds.search(addr[1])) 
   {
     lcd.setCursor(0,0);
     lcd.print("No more addr");
     ds.reset_search();
     delay(250);
     return;
   }
   
} 

int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
char buf[20];

int cntr = RepeatTemp;

void loop() {
  
  // *** Part 1: Measure temperature ***
  //
  byte i, sensor;
  byte present = 0;
  byte data[12];
  
  if (cntr == RepeatTemp)
  {
    for ( sensor=0; sensor<MAX_DS1820_SENSORS; sensor++ )
     {
       if ( OneWire::crc8( addr[sensor], 7) != addr[sensor][7]) 
       {
         lcd.setCursor(0,0);
         lcd.print("CRC not valid");
         return;
       }
  
       if ( addr[sensor][0] != 0x10) 
       {
         lcd.setCursor(0,0);
         lcd.print("Not DS18S20 dev ");
         return;
       }
  
       ds.reset();
       ds.select(addr[sensor]);
       ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
       delay(1000);     // maybe 750ms is enough, maybe not
       // we might do a ds.depower() here, but the reset will take care of it.
  
       present = ds.reset();
       ds.select(addr[sensor]);    
       ds.write(0xBE);         // Read Scratchpad
  
       for ( i = 0; i < 9; i++) 
       {           // we need 9 bytes
         data[i] = ds.read();
       }
  
       LowByte = data[0];
       HighByte = data[1];
       TReading = (HighByte << 8) + LowByte;
       SignBit = TReading & 0x8000;  // test most sig bit -- only for C, not F
       if (SignBit) // negative
       {
         TReading = (TReading ^ 0xffff) + 1; // 2's comp
       }
       Tc_100 = (TReading*100/2);      
  
       Whole = Tc_100 / 100;  // separate off the whole and fractional portions
       Fract = Tc_100 % 100;
       
         if (MAX_DS1820_SENSORS == 1)
         {
            sprintf(buf, "%c%d.%d\337 ",SignBit ? '-' : ' ', Whole, Fract < 10 ? 0 : Fract);
         }
        else
         { 
            sprintf(buf, "%d:%c%d.%d\337C     ",sensor,SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract);
         }   
   
       lcd.setCursor(0,1); //sensor%LCD_HEIGHT);
       lcd.print(buf);     
     }
     cntr = 0;
  }
  //
  // *** Part 2: Measure distance ***
  //
 // establish variables for duration of the ping, 
 // and the distance result in centimeters:
 long duration;
 float cm;
 float c; // speed of sound
 // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
 // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
 pinMode(pingPin, OUTPUT); 
 digitalWrite(pingPin, LOW); 
 delayMicroseconds(2); 
 digitalWrite(pingPin, HIGH); 
 delayMicroseconds(10); 
 digitalWrite(pingPin, LOW); 
 // Read the signal from the sensor: a HIGH pulse whose
 // duration is the time (in microseconds) from the sending
 // of the ping to the reception of its echo off of an object.
 pinMode(inPin, INPUT); 
 duration = pulseIn(inPin, HIGH); 
 //
 // estimate speed of sound from temperature:
 //
 if (FIXEDSPEED == 1)
 {
   c = 10000.0/29; // Original value for c in code
 }
 else
 {
  c = 331.3 + 0.606*Tc_100/100;
 }
  
 cm = microsecondsToCentimeters(duration, c); 
 
 lcd.setCursor(0, 0);
 for (i = 0; i < LCD_WIDTH; i = i + 1) {
    lcd.print(" ");
  }
 lcd.setCursor(1, 0); 
 lcd.print(cm,1); lcd.print(" cm");  
 
 lcd.setCursor(8, 1);
 lcd.print(c,1); lcd.print("m/s");
 delay(100); // measure range every 0.1 seconds
 cntr++;
 
 } 

float microsecondsToCentimeters(long microseconds, float c) { 
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // -- actually 29 microsec/cm = 10000/29 = 344.8 m/s, ie 22.3 deg C
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds * c / 20000; 
} 
