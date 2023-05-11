//Very easy remote Detonator with Timer and Radio control modes
//Made by Enrique Lopez - enriquelopezcode.github.io
// Necessary libraries: LiquidCrystal_I2C + RF24 (https://nrf24.github.io/RF24/)

#include "Arduino.h"

// Include Wire library for I2C
#include <Wire.h>

// Include NewLiquidCrystal library for I2C
#include <LiquidCrystal_I2C.h>

//Include nrf24 library for radio modules
#include "RF24.h"

//Instantiate radio object
RF24 radio(8,10);

// Let these addresses be used for the pair
uint8_t addresses[][6] = { "1Node", "2Node" };

// This is what we receive from the other device (the transmitter), translated in commands
unsigned char data;


// Define LCD pinout
const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;


//LCD I2C adress
const int i2c_addr_lcd = 0x27;

//Define buttons and the Pin that receives voltage during "detonation"
#define switchMode 7
#define confirm 6
#define actionPin 9

//Rotary Encoder
#define inputCLK 3
#define inputDT 5
int currentStateCLK;
int previousStateCLK; 

//Bool that decides if the arduino is reset at the end of the loop
bool reset = 0;

//responsible for printing dots during the Homing process in mode 2
int point = 0;

// holds Information on what mode the detonator is in
int mode = 0;

//Variables so LCD and Rotary encoder can be updated at regular intervals
int runtime = 0;
int update = 0;
int update_sw = 0;

//is the Detonator homed? (Mode 2)
bool homed = 0;

//Number of seconds to detonation
int counter;

//Instantiate LCD object
LiquidCrystal_I2C lcd(i2c_addr_lcd, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE); //Initializing Crystal LCD



void setup() {

 //Begin I2C-Bus
  Wire.begin();
  

   //Set Button Pins as Inputs
  pinMode(switchMode, INPUT);
  pinMode(confirm, INPUT);
  pinMode(actionPin,OUTPUT);

  // Set encoder pins as inputs  
  pinMode(inputCLK,INPUT);
  pinMode(inputDT,INPUT);

  //Initializing LCD
  lcd.begin(16,2);
  lcd.print("Choose Mode:");
  lcd.setCursor(0, 2);
  lcd.print("Timer, Radio");
  
  
  // Read the initial state of inputCLK
   // Assign to previousStateCLK variable
  previousStateCLK = digitalRead(inputCLK);
 


  //Radio Mode

  //Initiate radio Object
  radio.begin();

   // Set the transmit power to lowest available to prevent power supply related issues
  radio.setPALevel(RF24_PA_MIN);

    // Set the speed of the transmission to the quickest available
  radio.setDataRate(RF24_2MBPS);

  // Channel not often used in my country
  radio.setChannel(124);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  delay(100);
  
}

void loop() {
  //current time passed since Activation
  runtime = millis();

  //Calls function that switches between modes
  modeCheck();

    // Read the current state of inputCLK
   currentStateCLK = digitalRead(inputCLK);
    
   // If the previous and the current state of the inputCLK are different then a pulse has occured
   if (currentStateCLK != previousStateCLK){ 
       
     // If the inputDT state is different than the inputCLK state then 
     // the encoder is rotating counterclockwise
     if (digitalRead(inputDT) != currentStateCLK) { 
       counter --;
     }
     else{
       counter ++;}
      
       } 
       

  
 
  switch (mode) {

  //Timer mode
  case 1:
  //Minimum amount of time is 5 seconds (you can change it if you want)
  if (counter < 5)
     {counter = 120;} 
     else if (counter > 120){
       counter = 5;} 

    //update LCD every 300 milliseconds
    if(runtime - update >= 300)
    {
      lcd.setCursor(3, 0); 
      lcd.print(String(counter) + " Seconds");
      update = runtime;
    }

    //Start timer when action button is pressed
    if (digitalRead(confirm)){
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("Action in");

    //countdown
    for(int i = counter; i>0;i--){
      runtime = millis();

    if(runtime - update >= 500)
    {
      lcd.clear();
      lcd.setCursor(3,0);
      lcd.print("Action in");
      update = runtime;
    }

    lcd.setCursor(6,1);
    lcd.print(String(i));
    delay(1000);
    }
    //Detonation and Reset
    action();
    reset = 1;
    break;    


    }
     


    
    delay(10);
    break;
    
  //Radio mode
  case 2:


  radio.startListening();

  if(!homed){
  //Waiting for homing signal
  if(runtime - update >= 1000)
    {
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("Homing");
      for(int i = point; i>0;i--){
      lcd.print(".");}
      point += 1;
      if(point>3){
        point = 0;      
    }
    update = runtime;}

   
      if(radio.available())
      {
        while(radio.available()){
        radio.read( &data, sizeof(char));
      }
        radio.stopListening();

        //If homing signal is received
        if(data == 1) // "1" is the Signal for Homing
        {
        data = 2; //"2" is sent back -> message received and homed
        radio.write(&data, sizeof(char));
        homed = 1;
        break;
    }
    
    }
   
  }

  if(homed){
    //Wait for detonation signal
    if(runtime - update >= 1000)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Waiting Signal");
      for(int i = point; i>0;i--){
      lcd.print(".");
      point += 1;
      if(point>3){
        point = 0;
      }
    }
    update = runtime;}

    if (radio.available()){
    while(radio.available()){
      radio.read( &data, sizeof(char));
    }
    radio.stopListening();

  


    if(data == 3){ //"3" is detonation signal
    action();}
    

    if(data == 4){ //"4" is signal for unhoming and disabling radio control
      homed = 0;
      reset = 1;
      lcd.clear();
      lcd.setCursor(4,0);
      lcd.print("UNHOMING");
      delay(2000);
      }

    if(data == 1){ //if something went wrong reset the homing process
      homed = 0;


    }
  }
  }
    break;
  } 
 
    
//Rotary Encoder
previousStateCLK = currentStateCLK;
 
//reset the lcd and box
if(reset){
  mode = 0;
  lcd.clear();
  lcd.print("Choose Mode:");
  lcd.setCursor(0, 2);
  lcd.print("Timer, Radio");
  reset = 0;
  
  
}}



//Detonation
void action(){
    lcd.clear();
    lcd.setCursor(5,0);
    lcd.print("KABOOM");    
    digitalWrite(actionPin, HIGH);
    delay(5000);
    digitalWrite(actionPin, LOW);
    

  
}



//switches between modes and handles lcd display
void modeCheck(){
  if (digitalRead(switchMode)) {
    mode = mode + 1;
    if (mode > 2)
     { 
       mode = 1;
       
     }
    lcd.clear();
    lcd.setCursor(2, 0);
  switch (mode) {

  case 1:
    lcd.print("Timer Mode");
    counter = 5;
    
    delay(1000);    
    break;

  case 2:
    lcd.print("Remote Mode");
    
    delay(1000);
    break;
    
  
}

lcd.clear();
}}
