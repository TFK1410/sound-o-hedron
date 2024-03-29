#define MARCH_REPL_BR_THRESH 100
#define MARCH_PULL_AMOUNT 50

CRGBArray<EDGE_LENGTH> target_march_edge[3];
CRGBArray<EDGE_LENGTH> current_march_edge[3];

void marchRepl(uint8_t marchReplNum, mode_data mdata) {
    uint8_t segmentFill = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
  
    float marchCenter = EDGE_LENGTH * mdata.curve / 255.0;

    float marchCenterDistance = 0;
    
    int16_t pBrightness = 0;
    for (int i=0; i < EDGE_LENGTH; i++) {
        marchCenterDistance = abs(marchCenter - i);
        pBrightness = quadwave8(255.0 * marchCenterDistance / EDGE_LENGTH + 128) + (segmentFill * 2.0 - 255);
        pBrightness = constrain(pBrightness, 0, 255);
        target_march_edge[marchReplNum][i] = ColorFromPalette(palette, 255.0 * i / EDGE_LENGTH, pBrightness, LINEARBLEND);
    }
  
    for (byte i=0; i < EDGE_LENGTH; i++) {
        fadeTowardColor(current_march_edge[marchReplNum][i], target_march_edge[marchReplNum][i], MARCH_PULL_AMOUNT);
    }

    replicate_edge(edges, current_march_edge[marchReplNum]);
    
}
