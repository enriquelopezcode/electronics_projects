//code for Remote Controller for Remote Detonator
//created by Enrique Lopez - enriquelopezcode.github.io
//Libraries: Rf24 (https://nrf24.github.io/RF24/)

//Include Libraries
#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>

//Instanitiate radio object
RF24 radio(8, 10); // First Number: CE Pin; Second Number: CSN Pin

//define Buttons and LED
#define home 4
#define confirm 3
#define led 7

byte addresses[][6] = {"1Node", "2Node"};

//Data that is going to be sent
unsigned char data = 0;

//is radio module homed?
bool homed = 0;

void setup() 
{
  //Set Inputs and Outputs
  pinMode(home,INPUT);
  pinMode(confirm,INPUT);
  pinMode(led, OUTPUT);

    // Initiate the radio object
  radio.begin();

  // Set the transmit power to lowest available to prevent power supply related issues
  radio.setPALevel(RF24_PA_MIN);

  // Set the speed of the transmission to the quickest available
  radio.setDataRate(RF24_2MBPS);

  // Use a channel unlikely to be used by Wifi, Microwave ovens etc
  radio.setChannel(124);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);

}


void loop()
{
  //Be safe so we can send data
  radio.stopListening(); 

  if(digitalRead(home) && !homed){
  
  data = 1; //Send Homing Signal "1"
  if (!radio.write( &data, sizeof(unsigned char) )) {
    Serial.println("No acknowledgement of transmission - receiving radio device connected?");    
  }
   // Now listen for a response
  radio.startListening();

  unsigned long started_waiting_at = millis();

  // Loop here until we get indication that some data is ready for us to read (or we time out)
  while ( ! radio.available() ) {

    // Oh dear, no response received within our timescale
    if (millis() - started_waiting_at > 500 ) {
      return;}}
  
    unsigned char dataRx;
    radio.read( &dataRx, sizeof(unsigned char) );

    Serial.print(data);

      if (dataRx == 2){ //Homing signal Received!
        homed = 1;
        digitalWrite(led,HIGH);
        delay(2000);
        
        }

      
    }
   
  if (homed){
    if (digitalRead(confirm)){
    data = 3; //Detonation Signal
    radio.write(&data,sizeof(data));
    digitalWrite(led,LOW);
    delay(1000);
    digitalWrite(led,HIGH);


    }
    if(digitalRead(home)){
      data = 4; //Unhoming
      radio.write(&data,sizeof(data));
      homed = 0;
      digitalWrite(led,LOW);
      

      }
    }
    delay(500);}

