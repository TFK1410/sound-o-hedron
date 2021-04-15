#define FLASH_PULL_AMOUNT 50

#define PATT_CENTER_INNER 0
#define PATT_CENTER_OUTER 1
#define PATT_FORWARD 2
#define PATT_BACKWARD 3

CRGBArray<EDGE_LENGTH> target_flash_edge[3];
CRGBArray<EDGE_LENGTH> current_flash_edge[3];

void flash(uint8_t flashNum, mode_data mdata) {
    uint8_t edgePattern = mdata.params[0];
    uint8_t edges = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    uint8_t pattern = map(edgePattern, 0, 255, 0, PATT_BACKWARD);

    uint8_t edge_length = EDGE_LENGTH;
    if (pattern == PATT_CENTER_INNER || pattern == PATT_CENTER_OUTER) {
        edge_length /= 2;
    }

    int8_t num_lit = mdata.curve * edge_length / 255;
    uint8_t lit_remainder = (mdata.curve * edge_length) % 255;
    int iter = 0;
    switch (pattern) {
        case PATT_CENTER_INNER:
            for(CRGB & pixel : target_flash_edge[flashNum]) {
                if ((iter < edge_length && num_lit > edge_length - iter - 1) || (iter > edge_length-1 && num_lit > iter - edge_length)) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), 255, LINEARBLEND);
                } else if (num_lit == edge_length - iter - 1 || (iter > edge_length-1 && num_lit == iter - edge_length)) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), lit_remainder, LINEARBLEND);
                } else {
                    pixel = CRGB::Black;
                }
                iter++;
            }
            break;
        case PATT_CENTER_OUTER:
            for(CRGB & pixel : target_flash_edge[flashNum]) {
                if (num_lit > iter || (iter > edge_length-1 && num_lit > EDGE_LENGTH - iter - 1)) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), 255, LINEARBLEND);
                } else if (num_lit == iter || (iter > edge_length-1 && num_lit == EDGE_LENGTH - iter - 1)) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), lit_remainder, LINEARBLEND);
                } else {
                    pixel = CRGB::Black;
                }
                iter++;
            }
            break;
        case PATT_FORWARD:
            for(CRGB & pixel : target_flash_edge[flashNum]) {
                if (num_lit > 0) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), 255, LINEARBLEND);
                } else if (num_lit == 0) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), lit_remainder, LINEARBLEND);
                } else {
                    pixel = CRGB::Black;
                }
                iter++;
                num_lit--;
            }
            break;
        case PATT_BACKWARD:
            for(CRGB & pixel : target_flash_edge[flashNum]) {
                if (num_lit > EDGE_LENGTH - iter - 1) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), 255, LINEARBLEND);
                } else if (num_lit == EDGE_LENGTH - iter - 1) {
                    pixel = ColorFromPalette(palette, iter * 255 / (EDGE_LENGTH-1), lit_remainder, LINEARBLEND);
                } else {
                    pixel = CRGB::Black;
                }
                iter++;
            }
            break;
        default:
            for(CRGB & pixel : target_flash_edge[flashNum]) { pixel = CRGB::Black; }
            break;
    }

    for (byte i=0; i < EDGE_LENGTH; i++) {
        fadeTowardColor(current_flash_edge[flashNum][i], target_flash_edge[flashNum][i], FLASH_PULL_AMOUNT);
    }

    replicate_edge(edges, current_flash_edge[flashNum]);
}
