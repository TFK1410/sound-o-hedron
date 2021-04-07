#define MAX_RIPPLES 70
#define RIPPLE_SPACING_BRI 5
#define RIPPLE_SPACING_PAL 12
#define RIPPLE_INITS_ITER_DELAY MAX_RIPPLES

struct RipplePosition {
    uint8_t edge;
    uint8_t pos;
    int bri;
};

uint8_t rippleInit = 1;

unsigned long tickRippleLast;
uint16_t initRippleIter = 0;

RipplePosition ripples[MAX_RIPPLES];

void dodecaRipple(mode_data mdata) {
    uint8_t numOfRipples = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
  
    if (rippleInit == 1) {
        for (int i = 0; i < MAX_RIPPLES; i++) { ripples[i] = (RipplePosition) {random8(NUM_EDGES), random8(EDGE_LENGTH), random8(255)}; }
        rippleInit = 0;
        tickRippleLast = millis();
        return;
    }

    // this is expressed in milliseconds per 1 value in the brightness map
    float tickRippleBrightness = map(logscale8[255-mdata.curve], 0, 255, 20, 70) / 10.0;

    uint8_t ticksRipple = (millis() - tickRippleLast) / tickRippleBrightness;
    if (ticksRipple > 0) { tickRippleLast = millis(); }

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);

    uint8_t selectedRipples = map(numOfRipples, 0, 255, 0, MAX_RIPPLES);
    if (edgeCount > 0) {
        for (int i = 0; i < selectedRipples; i++) {
            if (ripples[i].bri < -RIPPLE_SPACING_BRI * EDGE_LENGTH - 255 && initRippleIter == 0) {
                ripples[i].edge = random8(NUM_EDGES);
                ripples[i].pos = random8(EDGE_LENGTH);
                ripples[i].bri = 255;
                initRippleIter = RIPPLE_INITS_ITER_DELAY + 1;
            }
            
            if (ripples[i].pos >= 0) {
                for (int j = 0; j < EDGE_LENGTH; j++) {
                    int dist = i-j;
                    int pBrightness = ripples[i].bri + abs(dist) * RIPPLE_SPACING_BRI;
                    uint8_t rippleEdge = abs(edge_list[ripples[i].edge % edgeCount]) - 1;
                    if (pBrightness < 256 && pBrightness > 0) {
                         leds[EDGE_LENGTH * rippleEdge + j] += ColorFromPalette(palette, 128 + dist * RIPPLE_SPACING_PAL, logscale8[pBrightness], LINEARBLEND);
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
