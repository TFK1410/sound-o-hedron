uint8_t edgeHistory[EDGE_LENGTH];
unsigned long tickImpulseLast;

CRGBArray<EDGE_LENGTH> impulse_edge;

void impulse(uint8_t edges, uint8_t edgeSpeed, uint8_t value, CRGBPalette16 palette) {
    if (tickImpulseLast == 0){
        tickImpulseLast = millis();
        return;
    }
    edgeSpeed = map(edgeSpeed, 0, 255, 10, 33);
    if (millis() - tickImpulseLast > edgeSpeed) {
        tickImpulseLast = millis();

        memcpy(&edgeHistory[1], &edgeHistory[0], EDGE_LENGTH-1);
        edgeHistory[0] = value;
    }

    uint8_t i=0;
    for(CRGB & pixel : impulse_edge) { pixel = ColorFromPalette(palette, i * 255 / EDGE_LENGTH - 1, edgeHistory[i], LINEARBLEND); i++; } 

    uint8_t edge_list_num[5];
    get_edge_list_nums_from_dmx_byte(edges, &edge_list_num[0]);
    
    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        if (edge_list_num[edge_list_index] < 255) {
            for (int8_t *edge = comboEdgeGroups[edge_list_num[edge_list_index]]; *edge != 0; edge++) {
                
                int led_num_start = 0;
                
                int8_t edge_num = *edge;
                if (edge_num > 0) {
                    led_num_start = EDGE_LENGTH * (edge_num - 1);
                    leds(led_num_start, led_num_start + EDGE_LENGTH - 1) = impulse_edge;
                } else if (edge_num < 0) {
                    led_num_start = EDGE_LENGTH * (0 - edge_num - 1);
                    int j = 0;
                    for (int i = led_num_start + EDGE_LENGTH - 1; i >= led_num_start; i--) {
                        leds[i] = impulse_edge[j];
                        j++;
                    }
                }
            }
        }
    }
}
