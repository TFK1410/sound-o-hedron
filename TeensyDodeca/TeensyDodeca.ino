#include <FastLED.h>
#include <ResponsiveAnalogRead.h>
#include <i2c_device.h>
#include <i2c_driver_wire.h>
#include <i2c_driver.h>
#include <i2c_register_slave.h>

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
#define DMX_BYTES_COUNT 25 //first byte marking changes + 3 sections (first 3 bytes selecting modes): 7 channels each: 4 for control + 3 for color
#define DMX_UPDATES_PER_SECOND 60

#define MIN_BRIGHTNESS 10
#define MAX_BRIGHTNESS 150
#define DEADZONE_WIDTH 25
#define ANALOG_PIN     23 //A9

#define MODE_PIN       22 //A8
#define MODE_COUNT     5
#define OFF_MODE       0
#define DEMO_MODE      1
#define DMX_MODE       2
int currentMode = 0;

//#define TIME_DEBUG
#define DMX_DEBUG
#ifdef DMX_DEBUG
uint8_t dmx_params[5];
#endif

CRGBArray<NUM_LEDS> leds;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

char dmxData[DMX_BYTES_COUNT-1]{0};

ResponsiveAnalogRead brightnessAnalog(ANALOG_PIN, true);
int brightnessValue = (MIN_BRIGHTNESS + MAX_BRIGHTNESS) / 2;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Wire1.begin();                         // join i2c bus
    Serial.begin(9600);                    // start serial for output

    init_group_list();                     // init edge lists
  
    delay(3000); // power-up safety delay
    FastLED.addLeds<NUM_STRIPS, LED_TYPE, LED_MASTER_PIN, COLOR_ORDER>(leds, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(brightnessValue);
    
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
    
    digitalWrite(LED_BUILTIN, LOW);
}


void loop()
{

    EVERY_N_MILLISECONDS(1000 / DMX_UPDATES_PER_SECOND){
        ReadDMX();
    }

    EVERY_N_MILLISECONDS(1000 / UPDATES_PER_SECOND) {
      
#ifdef TIME_DEBUG
        unsigned long time = millis();
#endif

        ReadMode();

#ifdef DMX_DEBUG

        brightnessAnalog.update();
        dmx_params[currentMode] = map(brightnessAnalog.getValue(), 0, 1023, 0, 255);
        
        Blackout();
//        flash(0xC0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
//        impulse(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
//        strobe(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
        march(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], createPalette(map(dmx_params[3], 0, 255, 0, 50), 0, 0, 0));
//        dodecaBlink(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
//        dodecaRipple(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
//        dodecaGhosting(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], RainbowColors_p);
//        dodecaFill(dmx_params[0], dmx_params[1], dmx_params[2], RainbowColors_p);
//        marchRepl(0xF0 | map(dmx_params[0], 0, 255, 0, 0xF), dmx_params[1], dmx_params[2], createPalette(map(dmx_params[3], 0, 255, 0, 50), 0, 0, 0));
        
#else
        SetBrightness();
        switch (currentMode) {
          case OFF_MODE:
            Blackout();
            break;
          case DEMO_MODE:
            demo();
            break;
          case DMX_MODE:
            Blackout();
            brightnessAnalog.update();
//            flash(0xF0 | map(brightnessAnalog.getValue(), 0, 1023, 0, 0xF), 128, 200, RainbowColors_p);
            flash(0x92, 128, map(brightnessAnalog.getValue(), 0, 1023, 0, 0xFF), RainbowColors_p);
            break;
          case 3:
            MarkEdges();
            break;
          default:
            Blackout();
            break;    
        }
#endif
        
        FastLED.show();
        
#ifdef TIME_DEBUG
        Serial.println(millis() - time);
#endif
    }

}

void ReadMode(){
    currentMode = map(analogRead(MODE_PIN), 0, 1150, 0, MODE_COUNT-1);
}

void SetBrightness(){ 
    brightnessAnalog.update();  // update the brightness based on the potentiometer value
    int anVal = brightnessAnalog.getValue();
    if (anVal > DEADZONE_WIDTH && anVal < 1023 - DEADZONE_WIDTH) {
        brightnessValue = map(anVal, DEADZONE_WIDTH, 1023 - DEADZONE_WIDTH, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        FastLED.setBrightness(brightnessValue);
    }
}

void ReadDMX(){
    Wire1.requestFrom(SLAVE_ADDRESS, DMX_BYTES_COUNT);   // request DMX data from slave device
    int availableCount = Wire1.available();              // if ready read the data from the slave
    if (availableCount >= DMX_BYTES_COUNT) {
        if (Wire1.read() == 255) {                       // new DMX data is being sent
            for (int i = 0; i < DMX_BYTES_COUNT-1; i++) {
                dmxData[i] = Wire1.read();
            }
        } else {                                         // no new data - flush buffer
            while(Wire1.available()) {
                Wire1.read();
            }
        }
    }
}
