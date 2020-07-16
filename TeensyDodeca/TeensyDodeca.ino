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


CRGBArray<NUM_LEDS> leds;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

char dmxData[DMX_BYTES_COUNT-1]{0};

ResponsiveAnalogRead brightnessAnalog(ANALOG_PIN, true);
int brightnessValue = MIN_BRIGHTNESS;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Wire1.begin();                         // join i2c bus
    Serial.begin(9600);                    // start serial for output
  
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
        SetBrightness();
    }

    EVERY_N_MILLISECONDS(1000 / UPDATES_PER_SECOND) {
//        unsigned long time = millis();

        ChangePalettePeriodically();
        
        static uint8_t startIndex = 0;
        startIndex = startIndex + 1; /* motion speed */
        
        FillLEDsFromPaletteColors(startIndex);

//        Serial.println(millis() - time);
    }
    
    FastLED.show();
//    FastLED.delay(1000 / UPDATES_PER_SECOND);
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

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};



// Additional notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes.
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.
