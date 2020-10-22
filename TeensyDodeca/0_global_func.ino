void Blackout() {
    leds(0, NUM_LEDS) = CRGB::Black;
}

void MarkEdges() {
    for (byte k=0; k < 30; k++) {
        int led_num_start = 0;
        led_num_start = EDGE_LENGTH * k;
       
        if (k / 10 == 0) {
            leds(led_num_start, led_num_start + k % 10) = CRGB::Red;
        } else if (k / 10 == 1) {
            leds(led_num_start, led_num_start + k % 10) = CRGB::Green;
        } else if (k / 10 == 2) {
            leds(led_num_start, led_num_start + k % 10) = CRGB::Blue;
        }
    }
}

void get_edge_list_nums_from_dmx_byte(uint8_t edge_byte, uint8_t *edge_list_num) {
    for (int i = 0; i < 5; i++) { edge_list_num[i] = 255; }
    if (edge_byte == 0xFF) {
        edge_list_num[0] = 0;
        edge_list_num[1] = FACES_COUNT;
        edge_list_num[2] = 2*FACES_COUNT;
        edge_list_num[3] = FACES_COUNT / 2;
        edge_list_num[4] = FACES_COUNT * 3 / 2;
    } else {
        uint8_t main_face = edge_byte & 0x0F;
        if (main_face > 0 && main_face <= FACES_COUNT){
            if(edge_byte & 0x80) { // is the face itself selected
                edge_list_num[0] = main_face - 1;
            }
            if(edge_byte & 0x40) { // are the sparks of the face selected
                edge_list_num[1] = main_face - 1 + FACES_COUNT;
            }
            if(edge_byte & 0x20) { // is the ring of the face selected
                edge_list_num[2] = main_face - 1 + 2*FACES_COUNT;
            }
            if(edge_byte & 0x10) { // is the opposite face selected
                edge_list_num[3] = (main_face - 1 + FACES_COUNT * 3 / 2) % (FACES_COUNT);
            }
        }
    }
}
