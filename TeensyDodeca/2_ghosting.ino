#define MAX_GHOSTS 12
#define GHOST_SPACING_BRI 5
#define GHOST_SPACING_PAL 12
#define GHOST_TRACE_LENGTH 255 / GHOST_SPACING_BRI

unsigned long tickGhostingLast;

bool ghostInit = 1;

uint16_t ghostingPosOffset;
uint8_t ghostingBriOffset;

void dodecaGhosting(mode_data mdata) {
    uint8_t numOfGhosts = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    if (!ghostInit) {
        ghostInit = 0;
        tickGhostingLast = millis();
        return;
    }
    // this is expressed in milliseconds per 1 value in the brightness map
    float tickGhostBrightness = map(logscale8[255-mdata.curve], 0, 255, 15, 50) / 10.0;
//    Serial.println(tickGhostBrightness);

    uint16_t ticksGhost = (millis() - tickGhostingLast) / tickGhostBrightness;
//    Serial.println(ticksGhost);
//    Serial.println(tickGhostingLast);
//    Serial.println((millis() - tickGhostingLast));
    if (ticksGhost > 0) { tickGhostingLast = millis(); }

    ghostingBriOffset += ticksGhost;
    if (ghostingBriOffset > GHOST_SPACING_BRI) { 
        ghostingBriOffset = 0; 
        ghostingPosOffset++;
        if (ghostingPosOffset >= 600) { ghostingPosOffset = 0; } 
    }

    int8_t edge_list[NUM_LEDS / EDGE_LENGTH];
    uint8_t edgeCount = get_edge_list(edges, &edge_list[0]);
    uint16_t numOfLeds = EDGE_LENGTH * edgeCount;

    uint8_t selectedGhosts = map(numOfGhosts, 0, 255, 1, MAX_GHOSTS);
    if (edgeCount > 0) {
        for (int i = 0; i < selectedGhosts; i++) {
            int16_t pPos = (numOfLeds - 1) * i / (selectedGhosts + 1) + ghostingPosOffset;
            if (pPos > (numOfLeds-1)) { pPos -= numOfLeds; } 
            
            uint8_t pBrightness = 255 - ghostingBriOffset;
            uint8_t pPalette = 255;
            for (int j = 0; j < GHOST_TRACE_LENGTH; j++) {
                
                uint8_t edgeNum = pPos / EDGE_LENGTH;
                uint8_t edgeOffset = pPos % EDGE_LENGTH;
    
                if (edge_list[edgeNum] > 0) {
                    leds[EDGE_LENGTH * (edge_list[edgeNum] - 1) + edgeOffset] += ColorFromPalette(palette, pPalette, logscale8[pBrightness], LINEARBLEND);
                } else {
                    leds[EDGE_LENGTH * (0 - edge_list[edgeNum]) - edgeOffset - 1] += ColorFromPalette(palette, pPalette, logscale8[pBrightness], LINEARBLEND);
                }
                
                pBrightness -= GHOST_SPACING_BRI;
                pPalette -= GHOST_SPACING_PAL;
                pPos--;
                if (pPos < 0) { pPos = numOfLeds - 1; }
            }
        }
    }
}
