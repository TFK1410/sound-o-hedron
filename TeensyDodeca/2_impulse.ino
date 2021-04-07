uint8_t edgeHistory[EDGE_LENGTH+1];
unsigned long tickImpulseLast;
unsigned long tickImpulsePassed;
float tickImpulsePercentage;

CRGBArray<EDGE_LENGTH> impulse_edge;

void impulse(mode_data mdata) {
    uint8_t edgeSpeed = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    if (tickImpulseLast == 0){
        tickImpulseLast = millis();
        return;
    }
    edgeSpeed = 50 - map(edgeSpeed, 0, 255, 0, 49); // in ms per pixel moved
    
    tickImpulsePassed = millis() - tickImpulseLast;
    if (tickImpulsePassed > edgeSpeed) {
        tickImpulseLast = millis();
        tickImpulsePassed = tickImpulsePassed % edgeSpeed;

        memcpy(&edgeHistory[1], &edgeHistory[0], EDGE_LENGTH);
        edgeHistory[0] = mdata.curve;
    }
    tickImpulsePercentage = 1.0 * tickImpulsePassed / edgeSpeed;

    uint8_t i=0;
    for(CRGB & pixel : impulse_edge) { 
        float pBrightness = edgeHistory[i] * (1 - tickImpulsePercentage) + edgeHistory[i+1] * tickImpulsePercentage;
        pixel = ColorFromPalette(palette, i * 255 / (EDGE_LENGTH - 1), pBrightness, LINEARBLEND);
        i++;
    }

    replicate_edge(edges, impulse_edge);
}
