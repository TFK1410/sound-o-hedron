#define MAX_BLINKS 60
#define MAX_BLINK_FADE_SPEED 80

struct BlinkPosition {
    int16_t pos;
    int16_t bri;
};

bool blinkInit = false;

unsigned long tickBlinkLast;
unsigned long tickBlinkPassed;

BlinkPosition blinks[MAX_BLINKS];

void dodecaBlink(uint8_t edges, uint8_t numOfBlinks, uint8_t value, CRGBPalette16 palette) {
    if (blinkInit == false) {
        for (int i = 0; i < MAX_BLINKS; i++) { blinks[i] = (BlinkPosition) {-1, random8(255)}; }
        blinkInit = true;
        tickBlinkLast = millis();
        return;
    }

    // this is expressed in milliseconds per 1 value in the brightness map
    unsigned long tickBlinkBrightness = map(logscale8[255-value], 0, 255, 2, 100);

    uint8_t ticksBlink = (millis() - tickBlinkLast) / tickBlinkBrightness;
    if (ticksBlink > 0) { tickBlinkLast = millis(); }

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);

    uint8_t selectedBlinks = map(numOfBlinks, 0, 255, 0, MAX_BLINKS);
    if (edgeCount > 0) {
        for (int i = 0; i < selectedBlinks; i++) {
            if (blinks[i].bri < 1) {
                blinks[i].pos = EDGE_LENGTH * (abs(edge_list[random8(edgeCount)]) - 1) + random8(EDGE_LENGTH);
                blinks[i].bri = 255 - random8(20);
    //            Serial.println(blinks[i].bri);
            } 
            if (blinks[i].pos > 0 && blinks[i].bri > 0) {
                leds[blinks[i].pos] = ColorFromPalette(palette, 255 / selectedBlinks * i, logscale8[blinks[i].bri], LINEARBLEND);
            }
    
            blinks[i].bri -= ticksBlink;
            // 25% chance for noise
            blinks[i].bri -= random8(4)/3;
        }
    }

    
}
