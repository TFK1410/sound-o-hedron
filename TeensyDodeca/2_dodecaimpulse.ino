//uint8_t dodecaHistory[5*EDGE_LENGTH+1];
CircularBuffer<uint8_t,5*EDGE_LENGTH+1> dodecaHistory;
unsigned long tIDL; // tickImpulseDodecaLast
unsigned long tIDP; // tickImpulseDodecaPassed
float tickImpulseDodecaPercentage;

#define TDIMIN 4000
#define TDIMAX 35000

void dodecaImpulse(mode_data mdata) {
    uint8_t dodecaSpeed = mdata.params[0];
    uint8_t frontFace = mdata.params[1];
    CRGBPalette16 palette = createPalette(mdata.params[2], mdata.color);
    
    if (tIDL == 0){
        tIDL = micros();
        return;
    }
    unsigned long dodecaSpeedLong = TDIMAX - dodecaSpeed * (TDIMAX - TDIMIN) / 255; // in microseconds per pixel moved
    
    tIDP += micros() - tIDL;
    tIDL = micros();
    uint8_t increments = tIDP / dodecaSpeedLong;
    if (increments > 5) {
        tIDP = 0;
//        memset(&dodecaHistory[0], 0, 5*EDGE_LENGTH+1);
        dodecaHistory.clear();
    } else if (increments > 0) {
        tIDP %= dodecaSpeedLong;

//        memcpy(&dodecaHistory[increments], &dodecaHistory[0], 5*EDGE_LENGTH+2-increments);
        for (int i = 0; i < increments; i++) {
//            dodecaHistory[i] = mdata.curve;
            dodecaHistory.unshift(mdata.curve);
        }
    }
    tickImpulseDodecaPercentage = 1.0 * tIDP / dodecaSpeedLong;

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
                
                ///////////////////////////////////////////
                //Skipping broken 400 which doesn't exist (because I wired the data lines at the beggining of 2x edges to the backup hence the + 1
                if (edge_num >= 20) { led_num_start++; }
                
                for (int i = 0; i < EDGE_LENGTH; i++) { 
                    uint16_t historyIndex = EDGE_LENGTH*edge_list_index+i;
                    float pBrightness = dodecaHistory[historyIndex] * (1 - tickImpulseDodecaPercentage) + dodecaHistory[historyIndex+1] * tickImpulseDodecaPercentage;
                    leds[led_num_start + i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND); 
                }
            } else if (edge_num < 0) {
                led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;

                ///////////////////////////////////////////
                //Skipping broken 400 which doesn't exist (because I wired the data lines at the beggining of 2x edges to the backup hence the + 1
                if (edge_num <= -20) { led_num_start++; }
            
                for (int i = 0; i < EDGE_LENGTH; i++) { 
                    uint16_t historyIndex = EDGE_LENGTH*edge_list_index+i;
                    float pBrightness = dodecaHistory[historyIndex] * (1 - tickImpulseDodecaPercentage) + dodecaHistory[historyIndex+1] * tickImpulseDodecaPercentage;
                    leds[led_num_start - i] = ColorFromPalette(palette, 255 * (edge_list_index * EDGE_LENGTH + i) / 5 / EDGE_LENGTH, pBrightness, LINEARBLEND);
                }
            }
        }
    }
}
