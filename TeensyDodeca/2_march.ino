#define MARCH_BR_THRESH 100

void march(mode_data mdata) {
    uint8_t segmentNum = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    segmentNum = map(segmentNum, 0, 255, 1, 30);

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);

    uint16_t selectedLedCount = EDGE_LENGTH * edgeCount;

    //finding pallete margin value
    uint8_t paletteMargin = 0;
    for (int i = 0; i < 256; i++) {
        if (quadwave8(i) > MARCH_BR_THRESH) {
            paletteMargin = triwave8(i);
            break;
        }
    }
    uint8_t paletteValue = 0;

    static float ledIncrementStart = 0;
    if (ledIncrementStart > selectedLedCount) { 
        ledIncrementStart = 0;
    } else {
        ledIncrementStart = ledIncrementStart + 2.0 * logscaleFloat8[mdata.curve] / 255;
    }
    
    uint8_t pBrightness = 0;
    float ledIncrement = ledIncrementStart;
    for (int i=0; i < edgeCount; i++) {
        
        int led_num_start = 0;
        uint8_t trigIterator = 0;
        
        int8_t edge_num = edge_list[i];
        if (edge_num > 0) {
            led_num_start = EDGE_LENGTH * (edge_num - 1);
            for (int j = led_num_start; j < led_num_start + EDGE_LENGTH; j++) {
                trigIterator = 255 * segmentNum * ledIncrement / selectedLedCount;
                pBrightness = quadwave8(trigIterator);
                
                if (pBrightness > MARCH_BR_THRESH) { 
                    pBrightness = map(pBrightness - MARCH_BR_THRESH, 0, 255 - MARCH_BR_THRESH, 0, 255);
                    paletteValue = map(triwave8(trigIterator) - paletteMargin, 0, 255 - paletteMargin, 0, 127);
                    if (triwave8(trigIterator+2) - triwave8(trigIterator) < 0) { paletteValue = 255 - paletteValue; }
                } else {
                    paletteValue = 0;
                    pBrightness = 0;
                }
                leds[j] = ColorFromPalette(palette, paletteValue, logscale8[pBrightness], LINEARBLEND);
                
                ledIncrement += 1;
                if (ledIncrement > selectedLedCount) { ledIncrement -= selectedLedCount; }
            }
        } else if (edge_num < 0) {
            led_num_start = EDGE_LENGTH * (0 - edge_num - 1);
            for (int j = led_num_start + EDGE_LENGTH - 1; j >= led_num_start; j--) {
                trigIterator = 255 * segmentNum * ledIncrement / selectedLedCount;
                pBrightness = quadwave8(trigIterator);
                
                if (pBrightness > MARCH_BR_THRESH) { 
                    pBrightness = map(pBrightness - MARCH_BR_THRESH, 0, 255 - MARCH_BR_THRESH, 0, 255);
                    paletteValue = map(triwave8(trigIterator) - paletteMargin, 0, 255 - paletteMargin, 0, 127);
                    if (triwave8(trigIterator+2) - triwave8(trigIterator) < 0) { paletteValue = 255 - paletteValue; }
                } else {
                    paletteValue = 0;
                    pBrightness = 0;
                }
                leds[j] = ColorFromPalette(palette, paletteValue, logscale8[pBrightness], LINEARBLEND);
                
                ledIncrement += 1;
                if (ledIncrement > selectedLedCount) { ledIncrement -= selectedLedCount; }
            }
        }
    }
    
}
