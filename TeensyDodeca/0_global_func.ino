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

void get_all_edges_from_face(uint8_t front_face, uint8_t *face_list_num) {
    face_list_num[0] = front_face;
    face_list_num[1] = FACES_COUNT + front_face;
    face_list_num[2] = 2*FACES_COUNT + front_face;
    face_list_num[3] = FACES_COUNT + (FACES_COUNT / 2 + front_face) % FACES_COUNT;
    face_list_num[4] = (FACES_COUNT / 2 + front_face) % FACES_COUNT;
}

void get_face_list_nums_from_dmx_byte(uint8_t edge_byte, uint8_t *face_list_num) {
    for (int i = 0; i < 5; i++) { face_list_num[i] = 255; }
    if (edge_byte == 0xFF) {
        get_all_edges_from_face(0, face_list_num);
    } else {
        uint8_t main_face = edge_byte & 0x0F;
        if (main_face > 0 && main_face <= FACES_COUNT){
            main_face = (main_face + front_face_offset) % FACES_COUNT;
            if(edge_byte & 0x80) { // is the face itself selected
                face_list_num[0] = main_face - 1;
            }
            if(edge_byte & 0x40) { // are the sparks of the face selected
                face_list_num[1] = main_face - 1 + FACES_COUNT;
            }
            if(edge_byte & 0x20) { // is the ring of the face selected
                face_list_num[2] = main_face - 1 + 2*FACES_COUNT;
            }
            if(edge_byte & 0x10) { // is the opposite face selected
                face_list_num[3] = (main_face - 1 + FACES_COUNT / 2) % (FACES_COUNT);
            }
        }
    }
}

// Copies over single edge template over every other edge based on the template
void replicate_edge(uint8_t edgeByte, CRGBArray<EDGE_LENGTH> edge_template){
    uint8_t face_list_num[5];
    get_face_list_nums_from_dmx_byte(edgeByte, &face_list_num[0]);
    
    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        if (face_list_num[edge_list_index] < 255) {
            for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
                
                int led_num_start = 0;
                
                int8_t edge_num = *edge;
                if (edge_num > 0) {
                    led_num_start = EDGE_LENGTH * (edge_num - 1);
                    int j = 0;
                    for (int i = led_num_start; i < led_num_start + EDGE_LENGTH; i++) {
                        leds[i] = edge_template[j];
                        j++;
                    }
                } else if (edge_num < 0) {
                    led_num_start = EDGE_LENGTH * (0 - edge_num - 1);
                    int j = EDGE_LENGTH - 1;
                    for (int i = led_num_start; i < led_num_start + EDGE_LENGTH; i++) {
                        leds[i] = edge_template[j];
                        j--;
                    }
                }
            }
        }
    }
}

// Creates a list of all the edges from the selected faces, returns the count of edges
uint8_t get_edge_list(uint8_t edgeByte, int8_t *edge_list) {
    uint8_t face_list_num[5];
    get_face_list_nums_from_dmx_byte(edgeByte, &face_list_num[0]);

    uint8_t i=0;
    
    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        if (face_list_num[edge_list_index] < 255) {
            for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
                edge_list[i] = *edge;
                i++;
            }
        }
    }
    return i;
}



// Blend one CRGB color toward another CRGB color by a given amount.
// Blending is linear, and done in the RGB color space.
// This function modifies 'cur' in place.
CRGB fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount)
{
  nblendU8TowardU8( cur.red,   target.red,   amount);
  nblendU8TowardU8( cur.green, target.green, amount);
  nblendU8TowardU8( cur.blue,  target.blue,  amount);
  return cur;
}

// Helper function that blends one uint8_t toward another by a given amount
void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
  if( cur == target) return;
 
  if( cur < target ) {
    uint8_t delta = target - cur;
    delta = scale8_video( delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video( delta, amount);
    cur -= delta;
  }
}


void nblendU16TowardU16( uint16_t& cur, const uint16_t target, uint16_t amount)
{
  if( cur == target) return;
 
  if( cur < target ) {
    uint16_t delta = target - cur;
    delta = scale16( delta, amount);
    delta = (delta == 0) ? 1 : delta;
    cur += delta;
  } else {
    uint16_t delta = cur - target;
    delta = scale16( delta, amount);
    delta = (delta == 0) ? 1 : delta;
    cur -= delta;
  }
}
