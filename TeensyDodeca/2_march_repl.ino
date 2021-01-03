#define MARCH_REPL_BR_THRESH 100
#define MARCH_PULL_AMOUNT 50

CRGBArray<EDGE_LENGTH> target_march_edge;
CRGBArray<EDGE_LENGTH> current_march_edge;

void marchRepl(uint8_t edges, uint8_t segmentFill, uint8_t marchPhase, CRGBPalette16 palette) {
    float marchCenter = EDGE_LENGTH * marchPhase / 255.0;

    float marchCenterDistance = 0;
    
    int16_t pBrightness = 0;
    for (int i=0; i < EDGE_LENGTH; i++) {
        marchCenterDistance = abs(marchCenter - i);
        pBrightness = quadwave8(255.0 * marchCenterDistance / EDGE_LENGTH + 128) + (segmentFill * 2.0 - 255);
        pBrightness = constrain(pBrightness, 0, 255);
        target_march_edge[i] = ColorFromPalette(palette, 255.0 * i / EDGE_LENGTH, pBrightness, LINEARBLEND);
    }
  
    for (byte i=0; i < EDGE_LENGTH; i++) {
        fadeTowardColor(current_march_edge[i], target_march_edge[i], MARCH_PULL_AMOUNT);
    }

    replicate_edge(edges, current_march_edge);
    
}
