#include <Wire.h>
#include "DmxInput.h"
DmxInput myDmxInput;

//#include <DMXSerial.h>

#define SLAVE_ADDRESS 0x01
#define START_CHANNEL 13
#define COUNT_CHANNEL 25
#define DMX_PIN 0
volatile uint8_t buffer[DMXINPUT_BUFFER_SIZE(START_CHANNEL, COUNT_CHANNEL+1)];
unsigned long lastUpdate = 0;
char dmxs[COUNT_CHANNEL + 1];


void setup()
{
  Wire.begin(SLAVE_ADDRESS); // join i2c bus
  Wire.onRequest(requestEvent); // register event
  
  myDmxInput.begin(DMX_PIN, START_CHANNEL, COUNT_CHANNEL);
  myDmxInput.read_async(buffer);
  
//  DMXSerial.init(DMXReceiver);
}

void loop()
{
  delay(1000000);
}

void requestEvent() {
  dmxs[0] = 0;
  
  if (lastUpdate < myDmxInput.latest_packet_timestamp()) {
    lastUpdate = millis();
    dmxs[0] = 255;
//    memcpy(&dmxs[1], buffer, COUNT_CHANNEL);
    
    for (int i = 0; i < COUNT_CHANNEL; i++) {
      dmxs[i+1] = buffer[i];
    }
  }
  
  Wire.write(dmxs, COUNT_CHANNEL + 1);

  
//  if (DMXSerial.dataUpdated()){
//    dmxs[0] = 255;
//    DMXSerial.resetUpdated();
//  }
//  else
//    dmxs[0] = 0;
//
//  for (int i = 0; i < COUNT_CHANNEL; i++) {
//    dmxs[i+1] = DMXSerial.read(START_CHANNEL + i);
//  }
//
//  Wire.write(dmxs, COUNT_CHANNEL + 1);
}
