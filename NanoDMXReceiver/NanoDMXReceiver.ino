#include <Wire.h>
#include <DMXSerial.h>

#define SLAVE_ADDRESS 0x01
#define START_CHANNEL 8
#define COUNT_CHANNEL 24
char dmxs[COUNT_CHANNEL + 1];

void setup()
{
  Wire.begin(SLAVE_ADDRESS); // join i2c bus
  Wire.onRequest(requestEvent); // register event
  DMXSerial.init(DMXReceiver);
}

void loop()
{
  delay(10000);
}

void requestEvent() {
  if (DMXSerial.dataUpdated()){
    dmxs[0] = 255;
    DMXSerial.resetUpdated();
  }
  else
    dmxs[0] = 0;

  for (int i = 0; i < COUNT_CHANNEL; i++) {
    dmxs[i+1] = DMXSerial.read(START_CHANNEL + i);
  }

  Wire.write(dmxs, COUNT_CHANNEL + 1);
}
