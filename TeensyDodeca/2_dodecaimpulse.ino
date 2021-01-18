uint8_t dodecaHistory[5*EDGE_LENGTH+1];
unsigned long tickImpulseDodecaLast;
unsigned long tickImpulseDodecaPassed;
float tickImpulseDodecaPercentage;

void dodecaImpulse(uint8_t frontFace, uint8_t dodecaSpeed, uint8_t value, CRGBPalette16 palette) {
    if (tickImpulseDodecaLast == 0){
        tickImpulseDodecaLast = millis();
        return;
    }
    dodecaSpeed = 50 - map(dodecaSpeed, 0, 255, 0, 49); // in ms per pixel moved
    
    tickImpulseDodecaPassed = millis() - tickImpulseDodecaLast;
    if (tickImpulseDodecaPassed > dodecaSpeed) {
        tickImpulseDodecaLast = millis();
        tickImpulseDodecaPassed = tickImpulseDodecaPassed % dodecaSpeed;

        memcpy(&dodecaHistory[1], &dodecaHistory[0], 5*EDGE_LENGTH);
        dodecaHistory[0] = value;
    }
    tickImpulseDodecaPercentage = 1.0 * tickImpulseDodecaPassed / dodecaSpeed;

    frontFace = map(frontFace, 0, 255, 0, FACES_COUNT - 1);

    uint8_t face_list_num[5];
    get_all_edges_from_face(frontFace, &face_list_num[0]);

    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        uint8_t flip=0;
        for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
            
            int led_num_start = 0;
            int8_t edge_num = *edge;
            if (edge_list_index == 2 && flip % 2 == 0) { edge_num = 0-edge_num; }// invert the directionality on the half of the ring edges
            flip++;
            if (edge_list_index == 3) { edge_num = 0-edge_num; }// invert the directionality on the opposite sparks part
            
            if (edge_num > 0) {
                led_num_start = EDGE_LENGTH * (edge_num - 1);
                for (int i = 0; i < EDGE_LENGTH; i++) { 
                    float pBrightness = dodecaHistory[EDGE_LENGTH*edge_list_index+i] * (1 - tickImpulseDodecaPercentage) + dodecaHistory[EDGE_LENGTH*edge_list_index+i+1] * tickImpulseDodecaPercentage;
                    leds[led_num_start + i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND); 
                }
            } else if (edge_num < 0) {
                led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                for (int i = 0; i < EDGE_LENGTH; i++) { 
                    float pBrightness = dodecaHistory[EDGE_LENGTH*edge_list_index+i] * (1 - tickImpulseDodecaPercentage) + dodecaHistory[EDGE_LENGTH*edge_list_index+i+1] * tickImpulseDodecaPercentage;
                    leds[led_num_start - i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND);
                }
            }
        }
    }
}
