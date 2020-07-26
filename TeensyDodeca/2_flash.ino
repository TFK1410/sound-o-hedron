#define FACES_COUNT 12
#define LIST_LEN 6

uint8_t faces[FACES_COUNT][LIST_LEN]{{0,  1,  2,  3,  20, 255},
                                     {10, 11, 12, 13, 0,  255},
                                     {20, 21, 22, 23, 10, 255},
                                     {4,  5,  6,  21, 3,  255},
                                     {14, 15, 16, 1,  13, 255},
                                     {24, 25, 26, 11, 23, 255},
                                     {2,  6,  7,  8,  14, 255},
                                     {12, 16, 17, 18, 24, 255},
                                     {22, 26, 27, 28, 4,  255},
                                     {5,  7,  9,  28, 29, 255},
                                     {15, 17, 19, 8,  9,  255},
                                     {25, 27, 29, 18, 19, 255}};

#define PATT_CENTER 0
#define PATT_FORWARD 1
#define PATT_BACKWARD 2
                     
void flash(uint8_t edges, uint8_t edgePattern, uint8_t blackout, uint8_t value, CRGB color) {
    uint8_t edge_list_num = map(edges, 0, 255, 0, FACES_COUNT);
    uint8_t pattern = map(edgePattern, 0, 255, 0, PATT_BACKWARD);
    uint8_t num_lit = map(value, 0, 255, 0, EDGE_LENGTH);
    
    uint8_t blackout_val = map(blackout, 0, 255, 0, 50);
    if (blackout_val == 0) blackout_val = 255;

    uint8_t edge_start = 0;
    uint8_t edge_end = EDGE_LENGTH;

    if (num_lit > 0) {
        if (pattern == PATT_CENTER) {
            edge_start = (EDGE_LENGTH - num_lit) / 2;
            edge_end = (EDGE_LENGTH + num_lit) / 2 - 1;
        } else if (pattern == PATT_FORWARD) {
            edge_end = num_lit - 1;
        } else if (pattern == PATT_BACKWARD) {
            edge_start = EDGE_LENGTH + 1 - num_lit;
        }
    
        for (byte k=0; faces[edge_list_num][k] < 255; k++) {
            int led_num_start = EDGE_LENGTH * faces[edge_list_num][k];
            leds(led_num_start, led_num_start + EDGE_LENGTH).fadeToBlackBy(blackout_val);
            leds(led_num_start + edge_start, led_num_start + edge_end) = color;
        } 
    } else {
        for (byte k=0; faces[edge_list_num][k] < 255; k++) {
            int led_num_start = EDGE_LENGTH * faces[edge_list_num][k];
            leds(led_num_start, led_num_start + EDGE_LENGTH).fadeToBlackBy(blackout_val);
        }
    }
}
