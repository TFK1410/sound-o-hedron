#define FLASH_PULL_AMOUNT 50

#define PATT_CENTER 0
#define PATT_FORWARD 1
#define PATT_BACKWARD 2

CRGBArray<EDGE_LENGTH> target_flash_edge[3];
CRGBArray<EDGE_LENGTH> current_flash_edge[3];

void flash(uint8_t flashNum, mode_data mdata) {
    uint8_t edgePattern = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    uint8_t pattern = map(edgePattern, 0, 255, 0, PATT_BACKWARD);

    uint8_t num_lit = mdata.curve * (EDGE_LENGTH + 1) / 255;
    uint8_t lit_remainder = (mdata.curve * (EDGE_LENGTH + 1)) % 255;
    CRGB faded_color = ColorFromPalette(palette, (num_lit + 1) * 255 / EDGE_LENGTH - 1, lit_remainder, LINEARBLEND);

    uint8_t edge_start = 0;
    uint8_t edge_end = EDGE_LENGTH;

    for(CRGB & pixel : target_flash_edge[flashNum]) { pixel = CRGB::Black; }
    if (num_lit > 0) {
        
        if (num_lit >= EDGE_LENGTH) {} 
        else if (pattern == PATT_CENTER) {
            edge_start = (EDGE_LENGTH - num_lit) / 2;
            edge_end = (EDGE_LENGTH + num_lit) / 2 - 1;
            
            if (num_lit % 2 == 0) {
                target_flash_edge[flashNum][edge_start - 1] = faded_color;
            } else {
                target_flash_edge[flashNum][edge_end + 1] = faded_color;
            }        
        } else if (pattern == PATT_FORWARD) {
            edge_end = num_lit - 1;
            target_flash_edge[flashNum][edge_end + 1] = faded_color;
        } else if (pattern == PATT_BACKWARD) {
            edge_start = EDGE_LENGTH + 1 - num_lit;
            target_flash_edge[flashNum][edge_start - 1] = faded_color;
        }

        for(int i = edge_start; i < edge_end; i++) { target_flash_edge[flashNum][i] = ColorFromPalette(palette, i * 255 / EDGE_LENGTH - 1, 255, LINEARBLEND); } 
    }

    for (byte i=0; i < EDGE_LENGTH; i++) {
        fadeTowardColor(current_flash_edge[flashNum][i], target_flash_edge[flashNum][i], FLASH_PULL_AMOUNT);
    }

    replicate_edge(edges, current_flash_edge[flashNum]);
}
