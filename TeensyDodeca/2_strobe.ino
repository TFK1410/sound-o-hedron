//unsigned long tickStrobe;

CRGBArray<EDGE_LENGTH> strobe_edge;

void strobe(uint8_t edges, uint8_t moveState, uint8_t value, CRGBPalette16 palette) {
//    moveSpeed = 255 - map(moveSpeed, 0, 255, 50, 255);
    
//    tickStrobe = millis() % (moveSpeed / 16 * moveSpeed); // check the current time to set the first color from the palette
  
//    uint8_t i = tickStrobe * (EDGE_LENGTH - 1) / (moveSpeed / 16 * moveSpeed);
    uint8_t i = map(moveState, 0, 255, 0, EDGE_LENGTH - 1);
    for(CRGB & pixel : strobe_edge) { 
        pixel = ColorFromPalette(palette, i * 255 / (EDGE_LENGTH - 1), value, LINEARBLEND); 
        i++;
        if (i == EDGE_LENGTH) {
            i = 0;
        }
    }
    
    replicate_edge(edges, strobe_edge);
}
