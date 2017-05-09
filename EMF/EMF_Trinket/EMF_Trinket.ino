#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;                                    //changes CS pin to digital 10 as per trinket pinout.
SdFile root;                                                  //microSD root.
boolean card_status;
boolean new_open;                                             //initializes flag for a newly opend file.
String fileName;                                              //file name variable. 
String dataString;                                            //data string variable.

unsigned long last = 0;
int n = 0;
int VLin;
int unsigned long VL2;
int VHin;
int unsigned long VH2;
int unsigned long V2;
int readings = 200;                                           //number of measurements to average over.
int unsigned long timeStamp;                                  //time variable. 
int LED[10]={3,4,5,6,8,14,15,16,17,18};                       //the sequential pins assigned to the LED bar graph.

void start_Up()                                               //the startup sequence to show device is operational.
{
  for(int i = 0; i < 5; i++)                       
  {
    for(int j = 0; j < 10; j++)                               //clear all LEDs
    {
      digitalWrite(LED[j], LOW);
    }
    digitalWrite(LED[i], HIGH);                               //light up LEDS 0-4 incrementing
    digitalWrite(LED[9-i], HIGH);                             //and LEDs 9-5 decrementing.
    delay(50);                                                //keep LEDs on for .05 sec.
  }
  for(int j = 0; j < 10; j++)                                 //clear all LEDs
  {
    digitalWrite(LED[j], LOW);
  }
}

int re_Bin(int x)                                             //function that reassigns a log2(x) value such that 
{                                                             //small measurements are higher resolution and 
  if (x <= 1){return 10;}                                     //higher measurement are lower resolution.
  if (x == 2){return 0;}
  if (x >= 3 and x <= 4){return 1;}
  if (x >= 5 and x <= 8){return 2;}
  if (x >= 9 and x <= 16){return 3;}
  if (x >= 17 and x <= 32){return 4;}
  if (x >= 33 and x <= 64){return 5;}
  if (x >= 65 and x <= 128){return 6;}
  if (x >= 129 and x <= 256){return 7;}
  if (x >= 257 and x <= 512){return 8;}
  if (x >= 513 and x <= 1023){return 9;}
}

int numberFile()                                              //determines the number appended to the file name.
{ 
int m = 1;                                                    //assumes file number appends 1 in case of empty folder.
  while(SD.exists("data"+String(m)+".csv") == true)           //iterate through files until the last is reacehed.
  {
   m++;
  }
  return(m);                                                  //return file number to append.
}

void setup() 
{
  Serial.begin(9600);
  
  analogReference(EXTERNAL);                                  //references readings to 1.056V/1024 resolution.
  pinMode(A6, INPUT);                                         //sets pin A6 to input only.
  pinMode(A7, INPUT);                                         //'' A7 ''.
  analogRead(A6);                                             //initializes pin A6 external reference.
  analogRead(A7);                                             //'' A7 ''.

  pinMode(9,INPUT_PULLUP);                                    //sets pin 9 as an active low digital input.

  for(int i = 0; i < 10; i++)                                 //sets all LED pins to output only.
  {
    pinMode(LED[i],OUTPUT);
  }

  start_Up();
  
  if(!SD.begin(chipSelect))                                   //alert the user if sd card not present.
  {
    for(int k = 0; k < 6; k++)                                //flash midle LEDs five times.
    {
      digitalWrite(LED[4],HIGH);
      digitalWrite(LED[5],HIGH);
      delay(100);
      digitalWrite(LED[4],LOW);
      digitalWrite(LED[5],LOW);
      delay(100);
    }
    card_status = false;                                      //set microSD false flag if not present.
    return;
  }
  else{card_status = true;}                                   //set microSD true flag otherwise.

  new_open = true;                                            //set new open flag true.
  int beginTime = millis();                                   //time in milliseconds when initialzation ends.
}

void loop() 
  {
  VLin = analogRead(A6);                                      //takes low freq. measurement.
  VLin = constrain(VLin,1,1023);                              //constrains low frq. to 2^10.
  VHin = analogRead(A7);                                      //takes highe freq. measurement.
  VHin = constrain(VHin,1,1023);                              //constrains high frq. to 2^10.
  if (VLin >= 1 or VHin >= 1)                                 //eleminates non-positive measurements.
  {
    VL2 = VL2 + VLin;                                         //running sum of low freq.
    VH2 = VH2 + VHin;                                         //running sum of high freq.
    n++;                                                      //tallies measurements.
  }
  if(n >= readings)                                           //once target # readings reached. 
  {
    timeStamp = millis();                                     //time of recording since main loop started
    if(card_status == true)                                   //if a microSD is present.
    {
      if(digitalRead(9) == LOW)                               //if record switch is toggled on.
      {
        if(new_open == true)                                  //if the record file is not newly opened.
        {
          fileName = "data"+String(numberFile())+".csv";      //set the new file name.
          new_open = false;                                   //set new open flag false.
        }
        File dataFile = SD.open(fileName, FILE_WRITE);        //open or create file with file name.
        dataString =                                          //create comma separated string of measurements 
          String(timeStamp)+                                  //and timestamp to write to file.
          ", "+String(VL2/n)+
          ", "+String(VH2/n);
        dataFile.println(dataString);                         //append string to file.
//        Serial.println(dataString);                     
        dataFile.close();                                     //close file to avoid corruption.
      }
      else{new_open = true;}                                  //set new open flag false if record swtich is toggled off.
    }
    V2 = max(VL2,VH2);                                        //choose highest value between low and high.
    int Vout = V2/n;                                          //average the highest sum
    int bar_Delimeter = re_Bin(Vout);                         //re-bin reading for easiest display.
    VL2 = 0;                                                  //reset all dynamic variables.
    VH2 = 0;
    V2 = 0;
    n = 0;
    if (bar_Delimeter == 10)                                  //if average below background emf (1/1024 V)
    {                                               
      for(int k = 0; k < 10; k++)                             //for all LED pins
      {
        digitalWrite(LED[k],LOW);                             //write low to clear display.
      }
    }
    else                                                      //if average above background emf
    {
      for(int i = 0; i < bar_Delimeter+1; i++)                //for log2(averaged readings)
      {
        digitalWrite(LED[i],HIGH);                            //light up LEDs
      }
      for(int j = bar_Delimeter+1; j < 10; j++)               //for the rest
      {
        digitalWrite(LED[j],LOW);                             //"clear" LEDs
      }
    }
  }
}
