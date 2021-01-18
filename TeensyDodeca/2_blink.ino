#define MAX_BLINKS 100
#define BLINK_INITS_ITER_DELAY 10

struct BlinkPosition {
    int16_t pos;
    int16_t bri;
};

bool blinkInit = 1;

unsigned long tickBlinkLast;
uint8_t initBlinkIter = 0;

BlinkPosition blinks[MAX_BLINKS];

void dodecaBlink(uint8_t edges, uint8_t numOfBlinks, uint8_t value, CRGBPalette16 palette) {
    if (!blinkInit) {
        for (int i = 0; i < MAX_BLINKS; i++) { blinks[i] = (BlinkPosition) {-1, 0}; }
        blinkInit = 0;
        tickBlinkLast = millis();
        return;
    }

    // this is expressed in milliseconds per 1 value in the brightness map
    float tickBlinkBrightness = map(logscale8[255-value], 0, 255, 10, 50) / 10.0;

    uint8_t ticksBlink = (millis() - tickBlinkLast) / tickBlinkBrightness;
    if (ticksBlink > 0) { tickBlinkLast = millis(); }

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);
    uint16_t ledCount = EDGE_LENGTH * edgeCount;

    uint8_t selectedBlinks = map(numOfBlinks, 0, 255, 0, MAX_BLINKS);
    if (edgeCount > 0) {
        for (int i = 0; i < selectedBlinks; i++) {
            if (blinks[i].bri < 1 && initBlinkIter == 0) {
                blinks[i].pos = random16(NUM_LEDS);
                blinks[i].bri = 255 - random8(20);
                initBlinkIter = BLINK_INITS_ITER_DELAY + 1;
            } 
            if (blinks[i].pos >= 0 && blinks[i].bri > 0) {
                uint16_t ledPos = blinks[i].pos % ledCount;
                uint16_t blinkPos = abs(edge_list[ledPos / EDGE_LENGTH]) - 1 + ledPos % EDGE_LENGTH;
                leds[blinkPos] = ColorFromPalette(palette, 255 / selectedBlinks * i, logscale8[blinks[i].bri], LINEARBLEND);
            }
    
            blinks[i].bri -= ticksBlink;
            // 25% chance for noise
            blinks[i].bri -= random8(4)/3;
            if (initBlinkIter > 0) { initBlinkIter--; }
        }
    }

    
}
