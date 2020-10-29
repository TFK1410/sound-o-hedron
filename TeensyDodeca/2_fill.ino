#define FILL_PULL_AMOUNT 50

uint8_t curFill;

void dodecaFill(uint8_t frontFace, uint8_t bri, uint8_t value, CRGBPalette16 palette) {
    frontFace = map(frontFace, 0, 255, 0, FACES_COUNT - 1);

    uint8_t face_list_num[5];
    get_all_edges_from_face(frontFace, &face_list_num[0]);

    nblendU8TowardU8(curFill, value, FILL_PULL_AMOUNT);
    float coloredPixels = curFill * 5 * EDGE_LENGTH / 255.0;
    
    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        if (coloredPixels > 0) {

            uint8_t litPixels = 0;
            float litRemainder = 0;
            if (coloredPixels > EDGE_LENGTH) {
                litPixels = EDGE_LENGTH;
            } else {
                litPixels = (uint8_t)coloredPixels;
                litRemainder = coloredPixels - litPixels;
            }
            coloredPixels -= EDGE_LENGTH;
            
            for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
                
                int led_num_start = 0;
                int8_t edge_num = *edge;
                if (edge_list_index == 3) { edge_num = 0-edge_num; }// invert the directionality on the opposite sparks part
                
                if (edge_num > 0) {
                    led_num_start = EDGE_LENGTH * (edge_num - 1);
                    for (int i = 0; i < litPixels; i++) { leds[led_num_start + i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, bri, LINEARBLEND); }
                    if (litRemainder > 0) { leds[led_num_start + litPixels] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + litPixels) / 5 / EDGE_LENGTH, bri * litRemainder, LINEARBLEND); } 
                } else if (edge_num < 0) {
                    led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                    for (int i = 0; i < litPixels; i++) { leds[led_num_start - i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, bri, LINEARBLEND); }
                    if (litRemainder > 0) { leds[led_num_start - litPixels] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + litPixels) / 5 / EDGE_LENGTH, bri * litRemainder, LINEARBLEND); } 
                }
            }
        } else {
            break;
        }
    }

    
}
