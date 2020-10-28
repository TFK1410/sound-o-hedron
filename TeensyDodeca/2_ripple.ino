#define MAX_RIPPLES 30
#define RIPPLE_SPACING_BRI 5
#define RIPPLE_SPACING_PAL 12
#define RIPPLE_INITS_ITER_DELAY MAX_RIPPLES * 5

struct RipplePosition {
    uint8_t edge;
    int8_t pos;
    int bri;
};

bool rippleInit = 1;

unsigned long tickRippleLast;
uint8_t initRippleIter = 0;

RipplePosition ripples[MAX_RIPPLES];

void dodecaRipple(uint8_t edges, uint8_t numOfRipples, uint8_t value, CRGBPalette16 palette) {
    if (!rippleInit) {
        for (int i = 0; i < MAX_RIPPLES; i++) { ripples[i] = (RipplePosition) {0, -1, -1000}; }
        rippleInit = 0;
        tickRippleLast = millis();
        return;
    }

    // this is expressed in milliseconds per 1 value in the brightness map
    float tickRippleBrightness = map(logscale8[255-value], 0, 255, 20, 70) / 10.0;

    uint8_t ticksRipple = (millis() - tickRippleLast) / tickRippleBrightness;
    if (ticksRipple > 0) { tickRippleLast = millis(); }

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);

    uint8_t selectedRipples = map(numOfRipples, 0, 255, 0, MAX_RIPPLES);
    if (edgeCount > 0) {
        for (int i = 0; i < selectedRipples; i++) {
            if (ripples[i].bri < -RIPPLE_SPACING_BRI * EDGE_LENGTH - 255 && initRippleIter == 0) {
                ripples[i].edge = abs(edge_list[random8(edgeCount)]) - 1;
                ripples[i].pos = random8(EDGE_LENGTH);
                ripples[i].bri = 255;
                initRippleIter = RIPPLE_INITS_ITER_DELAY + 1;
            }
            
            if (ripples[i].pos >= 0) {
                for (int j = 0; j < EDGE_LENGTH; j++) {
                    int dist = i-j;
                    int pBrightness = ripples[i].bri + abs(dist) * RIPPLE_SPACING_BRI;
                    if (pBrightness < 256 && pBrightness > 0) {
                         leds[EDGE_LENGTH * ripples[i].edge + j] += ColorFromPalette(palette, 128 + dist * RIPPLE_SPACING_PAL, logscale8[pBrightness], LINEARBLEND);
                    }
                }    
            }
    
            ripples[i].bri -= ticksRipple;
            // 25% chance for noise
            ripples[i].bri -= random8(4)/3;
            if (initRippleIter > 0) { initRippleIter--; }
        }
    }

    
}
