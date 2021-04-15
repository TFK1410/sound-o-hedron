uint8_t edgeHistory[EDGE_LENGTH+1];
unsigned long tIL; // tickImpulseLast
unsigned long tIP; // tickImpulsePassed
float tickImpulsePercentage;

CRGBArray<EDGE_LENGTH> impulse_edge;

#define TIMIN 4000
#define TIMAX 35000

void impulse(mode_data mdata) {
    uint8_t edgeSpeed = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    if (tIL == 0){
        tIL = millis();
        return;
    }
    
    unsigned long edgeSpeedLong = TIMAX - edgeSpeed * (TIMAX - TIMIN) / 255; // in microseconds per pixel moved

    tIP += micros() - tIL;
    tIL = micros();
    uint8_t increments = tIP / edgeSpeedLong;
    if (increments > 5) {
        tIP = 0;
        memset(&edgeHistory[0], 0, EDGE_LENGTH+1);
    } else if (increments > 0) {
        tIP %= edgeSpeedLong;

        memcpy(&edgeHistory[increments], &edgeHistory[0], EDGE_LENGTH+2-increments);
        for (int i = 0; i < increments; i++) {
            edgeHistory[i] = mdata.curve;
        }
    }
    tickImpulseDodecaPercentage = 1.0 * tIP / edgeSpeedLong;

    uint8_t i=0;
    for(CRGB & pixel : impulse_edge) { 
        float pBrightness = edgeHistory[i] * (1 - tickImpulsePercentage) + edgeHistory[i+1] * tickImpulsePercentage;
        pixel = ColorFromPalette(palette, i * 255 / (EDGE_LENGTH - 1), pBrightness, LINEARBLEND);
        i++;
    }

    replicate_edge(edges, impulse_edge);
}
