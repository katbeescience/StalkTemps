// This sketch currently writes false temperatures to the SD.
// It's been modified from a sketch originally by Corey Fowler at Ocean Controls.
// TCMux shield demo by Ocean Controls
// Sends the data to the SD.
// Type @NS1<cr> (don't type <cr> it's the carriage return character, just hit enter) to set the number of sensor to 1
// Type @NS8<cr> to set the number of sensors to 8
// Type @UD1<cr> to set the update delay to 1 second
// Type @SV<cr> to save the number of sensors and update delay variables to eeprom


#include "RTClib.h"
#include <string.h> //Use the string Library
#include <ctype.h>
#include <EEPROM.h>
#include <SD.h>
//#include <MAX31855.h>
#include <SPI.h> 

RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//#define SHOWMEYOURBITS // Display the raw 32bit binary data from the MAX31855

#define PINEN 7 //Mux Enable pin
#define PINA0 4 //Mux Address 0 pin
#define PINA1 5 //Mux Address 1 pin
#define PINA2 6 //Mux Address 2 pin
#define PINSO 12 //TCAmp Slave Out pin (MISO)
#define PINSC 13 //TCAmp Serial Clock (SCK)
#define PINCS 9  //TCAmp Chip Select Change this to match the position of the Chip Select Link
#define chipSelect 10 // Chipselect for the SD

int Temp[8], SensorFail[8];
float floatTemp, floatInternalTemp;
char failMode[8];
int internalTemp, intTempFrac;
unsigned int Mask;
//char data[16];
char i, j, NumSensors =8, UpdateDelay;
char Rxchar, Rxenable, Rxptr, Cmdcomplete, R;
char Rxbuf[32];
char adrbuf[3], cmdbuf[3], valbuf[12];
int val = 0, Param;     
unsigned long time;
File logfile;

void setup()   
{     
  //Serial.begin(9600);  
  //Serial.println("TCMUXV3");
  if (EEPROM.read(511)==1)
  {
    NumSensors = EEPROM.read(8);
    UpdateDelay = EEPROM.read(1);
  }
  pinMode(PINEN, OUTPUT);     
  pinMode(PINA0, OUTPUT);    
  pinMode(PINA1, OUTPUT);    
  pinMode(PINA2, OUTPUT);    
  pinMode(PINSO, INPUT);    
  pinMode(PINCS, OUTPUT);    
  pinMode(PINSC, OUTPUT);   
  pinMode(chipSelect, OUTPUT); 
  
  digitalWrite(PINEN, LOW);   // enable on
  digitalWrite(PINA0, LOW); // low, low, low = channel 1
  digitalWrite(PINA1, LOW); 
  digitalWrite(PINA2, LOW); 
  digitalWrite(PINSC, LOW); //put clock in low
  


  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }

  if (! rtc.begin()) {
   // Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.initialized()) {
    //Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

//SD INITIALIZATION

delay(10);
digitalWrite(PINCS, HIGH);
digitalWrite(chipSelect, LOW);
delay(10);

if (!SD.begin(chipSelect, 11, 12, 13)) {
     // Serial.print(F("Card failed\n")); // Show error if failed
    }
    else // Otherwise if initialization works
     // Serial.print(F("Card initialized\n"));
    delay(1500);

logfile = SD.open("logfile.txt", FILE_WRITE);
if (logfile.write("I AM THE LOGFILE INDEED\n") == 0){ Serial.println("ERROR: Could not write to file.");}
delay(1000);
logfile.close();

//if (!SD.exists("logfile.txt")){ Serial.print("File does not exist\n"); }
//else {Serial.print("File was created\n");}
delay(10);

digitalWrite(PINCS, LOW);
digitalWrite(chipSelect, HIGH);
delay(10);

}

// MAIN LOOP
void loop()                     
{
  DateTime now = rtc.now();

//  Serial.print(now.year(), DEC);
//  Serial.print('/');
//  Serial.print(now.month(), DEC);
//  Serial.print('/');
//  Serial.print(now.day(), DEC);
//  Serial.print(" (");
//  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
//  Serial.print(") ");
//  Serial.print(now.hour(), DEC);
//  Serial.print(':');
//  Serial.print(now.minute(), DEC);
//  Serial.print(':');
//  Serial.print(now.second(), DEC);
//  Serial.println();
  
  if (millis() > (time + ((unsigned int)UpdateDelay*1000)))
  {
digitalWrite(PINEN, HIGH);
    time = millis();
    //for(j=0;j<NumSensors;j++)
    //{
    if (j<(NumSensors-1)) j++;
    else j=0;
      
      switch (j) //select channel
      {
        case 0:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 1:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 2:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 3:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 4:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 5:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 6:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        case 7:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
      }
      
      delay(5);
      digitalWrite(PINCS, LOW); //stop conversion
      delay(5);
      digitalWrite(PINCS, HIGH); //begin conversion
      delay(100);  //wait 100 ms for conversion to complete
      digitalWrite(PINCS, LOW); //stop conversion, start serial interface
      delay(1);
      
      Temp[j] = 0;
      failMode[j] = 0;
      SensorFail[j] = 0;
      internalTemp = 0;
      for (i=31;i>=0;i--)
      {
          digitalWrite(PINSC, HIGH);
          delay(1);
          
           //print out bits
         #ifdef SHOWMEYOURBITS
         if (digitalRead(PINSO)==1)
          {
            //Serial.print("1");
          }
          else
          {
            //Serial.print("0");
          }
          #endif
          
        if ((i<=31) && (i>=18))
        {
          // these 14 bits are the thermocouple temperature data
          // bit 31 sign
          // bit 30 MSB = 2^10
          // bit 18 LSB = 2^-2 (0.25 degC)
          
          Mask = 1<<(i-18);
          if (digitalRead(PINSO)==1)
          {
            if (i == 31)
            {
              Temp[j] += (0b11<<14);//pad the temp with the bit 31 value so we can read negative values correctly
            }
            Temp[j] += Mask;
            //Serial.print("1");
          }
          else
          {
           // Serial.print("0");
          }
        }
        //bit 17 is reserved
        //bit 16 is sensor fault
        if (i==16)
        {
          SensorFail[j] = digitalRead(PINSO);
        }
        
        if ((i<=15) && (i>=4))
        {
          //these 12 bits are the internal temp of the chip
          //bit 15 sign
          //bit 14 MSB = 2^6
          //bit 4 LSB = 2^-4 (0.0625 degC)
          Mask = 1<<(i-4);
          if (digitalRead(PINSO)==1)
          {
            if (i == 15)
            {
              internalTemp += (0b1111<<12);//pad the temp with the bit 31 value so we can read negative values correctly
            }
            
            internalTemp += Mask;//should probably pad the temp with the bit 15 value so we can read negative values correctly
            //Serial.print("1");
          }
          else
          {
           // Serial.print("0");
          }
          
        }
        //bit 3 is reserved
        if (i==2)
        {
          failMode[j] += digitalRead(PINSO)<<2;//bit 2 is set if shorted to VCC
        }
        if (i==1)
        {
          failMode[j] += digitalRead(PINSO)<<1;//bit 1 is set if shorted to GND
        }
        if (i==0)
        {
          failMode[j] += digitalRead(PINSO)<<0;//bit 0 is set if open circuit
        }
        
        
        digitalWrite(PINSC, LOW);
        delay(1);
        //delay(1);
      }
      //Serial.println();
    
      //Serial.println(Temp,BIN);
//      Serial.print("#");
//      Serial.print(j+1,DEC);
//      Serial.print(": ");
      if (SensorFail[j] == 1)
      {
        //Serial.print("FAIL");
        if ((failMode[j] & 0b0100) == 0b0100)
        {
         // Serial.print(" SHORT TO VCC");
        }
        if ((failMode[j] & 0b0010) == 0b0010)
        {
         // Serial.print(" SHORT TO GND");
        }
        if ((failMode[j] & 0b0001) == 0b0001)
        {
         // Serial.print(" OPEN CIRCUIT");
        }
      }
      else
      {
        floatTemp = (float)Temp[j] * 0.25;
        //Serial.print(floatTemp,2);
        //This bit doesn't work for neg temps
        //Serial.print(" degC");
      //}
        //delay(1000);
    }//end reading sensors
    //Serial.println("");
   // Serial.print(" Int: ");
    floatInternalTemp = (float)internalTemp * 0.0625;
  //  Serial.print(floatInternalTemp,4);
    //This doesn't work for negative values
    
  //  Serial.print(" degC");
  //  Serial.println("");
   
  }//end time
//  if (Serial.available() > 0)    // Is a character waiting in the buffer?
//  {
//    Rxchar = Serial.read();      // Get the waiting character
//
//    if (Rxchar == '@')      // Can start recording after @ symbol
//    {
//      if (Cmdcomplete != 1)
//      {
//        Rxenable = 1;
//        Rxptr = 1;
//      }//end cmdcomplete
//    }//end rxchar
//    if (Rxenable == 1)           // its enabled so record the characters
//    {
//      if ((Rxchar != 32) && (Rxchar != '@')) //dont save the spaces or @ symbol
//      {
//        Rxbuf[Rxptr] = Rxchar;
//        //Serial.println(Rxchar);
//        Rxptr++;
//        if (Rxptr > 13) 
//        {
//          Rxenable = 0;
//        }//end rxptr
//      }//end rxchar
//      if (Rxchar == 13) 
//      {
//        Rxenable = 0;
//        Cmdcomplete = 1;
//      }//end rxchar
//    }//end rxenable
digitalWrite(PINEN, LOW);

//PRINTING TO SD CARD
         //digitalWrite(PINSC, HIGH);
         digitalWrite(PINCS, LOW);
         digitalWrite(chipSelect, HIGH);
         SD.open("logfile.txt", FILE_WRITE);
           if (j == 0){
            logfile.write(now.year());
            logfile.write('/');
            logfile.write(now.month());
            logfile.write('/');
            logfile.write(now.day());
            logfile.write("  ");
            logfile.write(now.hour());
            logfile.write(':');
            logfile.write(now.minute());
            logfile.write(':');
            logfile.write(now.second());
            logfile.write("\n");
          }
            logfile.write("#");
            logfile.write(j+1);
            logfile.write(": ");
            logfile.write(floatTemp);
            logfile.write(" degC");
            logfile.close();

         //digitalWrite(PINSC, HIGH);
         digitalWrite(PINCS, HIGH);
         digitalWrite(chipSelect, LOW);
          

  //}// end serial available


   
  if (Cmdcomplete == 1)
  {
    Cmdcomplete = 0;
     cmdbuf[0] = toupper(Rxbuf[1]); //copy and convert to upper case
     cmdbuf[1] = toupper(Rxbuf[2]); //copy and convert to upper case
     cmdbuf[2] = 0; //null terminate        Command = Chr(rxbuf(3)) + Chr(rxbuf(4))
     //   Command = Ucase(command)
  //Serial.println(cmdbuf);
     valbuf[0] = Rxbuf[3]; //        Mystr = Chr(rxbuf(5))
        R = Rxptr - 1;
            for (i = 4 ; i <= R ; i++)//For I = 6 To R
            {
                valbuf[i-3] = Rxbuf[i]; //Mystr = Mystr + Chr(rxbuf(i))
            }
     valbuf[R+1] = 0; //null terminate
     Param = atoi(valbuf);//   Param = Val(mystr)

     //Serial.println(Param); //   'Print "Parameter: " ; Param

       
              if (strcmp(cmdbuf,"NS")==0)       //NumSensors
              {
                   //'Print "command was ON"
                   if ((Param <= 8) && (Param > 0)) 
                   {
                      NumSensors = Param;                   
                   }
                   
              }
              if (strcmp(cmdbuf,"UD")==0)       //UpdateDelay
              {
                   //'Print "command was ON"
                   if ((Param <= 60) && (Param >= 0)) 
                   {
                      UpdateDelay = Param;                   
                   }
                   
              }
              if (strcmp(cmdbuf,"SV")==0)       //Save
              {
                   EEPROM.write(0,NumSensors);
                   EEPROM.write(1,UpdateDelay);
                   EEPROM.write(511,1);
              }
         }

}      
