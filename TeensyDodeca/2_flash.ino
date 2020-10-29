#define FLASH_PULL_AMOUNT 50

#define PATT_CENTER 0
#define PATT_FORWARD 1
#define PATT_BACKWARD 2

CRGBArray<EDGE_LENGTH> target_flash_edge;
CRGBArray<EDGE_LENGTH> current_flash_edge;
                     
void flash(uint8_t edges, uint8_t edgePattern, uint8_t value, CRGBPalette16 palette) {
    uint8_t pattern = map(edgePattern, 0, 255, 0, PATT_BACKWARD);

    uint8_t num_lit = value * (EDGE_LENGTH + 1) / 255;
    uint8_t lit_remainder = (value * (EDGE_LENGTH + 1)) % 255;
    CRGB faded_color = ColorFromPalette(palette, (num_lit + 1) * 255 / EDGE_LENGTH - 1, lit_remainder, LINEARBLEND);
//    faded_color %= lit_remainder;

    uint8_t edge_start = 0;
    uint8_t edge_end = EDGE_LENGTH;

    target_flash_edge(0, EDGE_LENGTH) = CRGB::Black;
    if (num_lit > 0) {
        
        if (num_lit >= EDGE_LENGTH) {
//            target_flash_edge(0, EDGE_LENGTH) = CRGB::Black;
        } else if (pattern == PATT_CENTER) {
            edge_start = (EDGE_LENGTH - num_lit) / 2;
            edge_end = (EDGE_LENGTH + num_lit) / 2 - 1;
            
            if (num_lit % 2 == 0) {
                target_flash_edge[edge_start - 1] = faded_color;
            } else {
                target_flash_edge[edge_end + 1] = faded_color;
            }        
        } else if (pattern == PATT_FORWARD) {
            edge_end = num_lit - 1;
            target_flash_edge[edge_end + 1] = faded_color;
        } else if (pattern == PATT_BACKWARD) {
            edge_start = EDGE_LENGTH + 1 - num_lit;
            target_flash_edge[edge_start - 1] = faded_color;
        }

        int i = 0;
        for(CRGB & pixel : target_flash_edge(edge_start, edge_end)) { pixel = ColorFromPalette(palette, i * 255 / EDGE_LENGTH - 1, 255, LINEARBLEND); i++; } 
//        target_flash_edge(edge_start, edge_end) = color;
    }

    for (byte i=0; i < EDGE_LENGTH; i++) {
        fadeTowardColor(current_flash_edge[i], target_flash_edge[i], FLASH_PULL_AMOUNT);
    }

    replicate_edge(edges, current_flash_edge);
}
