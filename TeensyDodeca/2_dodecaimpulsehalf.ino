uint8_t dodecaHalfHistory[5*EDGE_LENGTH/2+1];
unsigned long tickImpulseDodecaHLast;
unsigned long tickImpulseDodecaHalfPassed;
float tickImpulseDodecaHalfPercentage;

void dodecaImpulseHalf(mode_data mdata) {
    uint8_t dodecaSpeed = mdata.params[0];
    uint8_t frontFace = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    if (tickImpulseDodecaHLast == 0){
        tickImpulseDodecaHLast = millis();
        return;
    }
    dodecaSpeed = 50 - map(dodecaSpeed, 0, 255, 0, 49); // in ms per pixel moved
    
    tickImpulseDodecaHalfPassed = millis() - tickImpulseDodecaHLast;
    if (tickImpulseDodecaHalfPassed > dodecaSpeed) {
        tickImpulseDodecaHLast = millis();
        tickImpulseDodecaHalfPassed = tickImpulseDodecaHalfPassed % dodecaSpeed;

        memcpy(&dodecaHalfHistory[1], &dodecaHalfHistory[0], 5*EDGE_LENGTH/2);
        dodecaHalfHistory[0] = mdata.curve;
    }
    tickImpulseDodecaHalfPercentage = 1.0 * tickImpulseDodecaHalfPassed / dodecaSpeed;

    frontFace = map(frontFace, 0, 255, 0, FACES_COUNT - 1);
    frontFace = (frontFace + front_face_offset) % FACES_COUNT;

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
                    int historyIndex = abs(EDGE_LENGTH*edge_list_index+i - 5*EDGE_LENGTH/2);
                    float pBrightness = dodecaHalfHistory[historyIndex] * (1 - tickImpulseDodecaHalfPercentage) + dodecaHalfHistory[historyIndex+1] * tickImpulseDodecaHalfPercentage;
                    leds[led_num_start + i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND); 
                }
            } else if (edge_num < 0) {
                led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                for (int i = 0; i < EDGE_LENGTH; i++) { 
                    int historyIndex = abs(EDGE_LENGTH*edge_list_index+i - 5*EDGE_LENGTH/2);
                    float pBrightness = dodecaHalfHistory[historyIndex] * (1 - tickImpulseDodecaHalfPercentage) + dodecaHalfHistory[historyIndex+1] * tickImpulseDodecaHalfPercentage;
                    leds[led_num_start - i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND);
                }
            }
        }
    }
}
