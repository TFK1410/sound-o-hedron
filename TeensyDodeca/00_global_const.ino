// The new Teensy 4 also supports parallel output, but it's done slightly differently from the above platforms. First off, there are three sets of possible pins that can be used - each of the three set of pins, in order:
//
//    First: 1,0,24,25,19,18,14,15,17,16,22,23,20,21,26,27
//    Second: 10,12,11,13,6,9,32,8,7
//    Third: 37, 36, 35, 34, 39, 38, 28, 31, 30

#define EDGE_LENGTH 20
#define NUM_LEDS_PER_STRIP 200
#define NUM_STRIPS         3
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS

#define LED_MASTER_PIN     18
#define LED_TYPE           WS2812
#define COLOR_ORDER        GRB
#define UPDATES_PER_SECOND 100

#define SLAVE_ADDRESS   0x01
#define DMX_BYTES_COUNT 22 //3 sections: 7 channels each: 4 for control + 3 for color and first byte marking changes 
#define DMX_UPDATES_PER_SECOND 60

#define MIN_BRIGHTNESS 10
#define MAX_BRIGHTNESS 150
#define DEADZONE_WIDTH 25
#define ANALOG_PIN     23 //A9
