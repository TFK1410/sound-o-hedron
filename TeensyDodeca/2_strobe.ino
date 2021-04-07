//unsigned long tickStrobe;

CRGBArray<EDGE_LENGTH> strobe_edge;

void strobe(mode_data mdata) {
    uint8_t moveState = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    uint8_t i = map(moveState, 0, 255, 0, EDGE_LENGTH - 1);
    for(CRGB & pixel : strobe_edge) { 
        pixel = ColorFromPalette(palette, i * 255 / (EDGE_LENGTH - 1), mdata.curve, LINEARBLEND); 
        i++;
        if (i == EDGE_LENGTH) {
            i = 0;
        }
    }
    
    replicate_edge(edges, strobe_edge);
}
