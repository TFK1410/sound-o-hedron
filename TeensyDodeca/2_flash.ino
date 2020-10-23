

#define PATT_CENTER 0
#define PATT_FORWARD 1
#define PATT_BACKWARD 2

CRGBArray<EDGE_LENGTH> target_flash_edge;
CRGBArray<EDGE_LENGTH> current_flash_edge;
                     
void flash(uint8_t edges, uint8_t edgePattern, uint8_t blackout, uint8_t value, CRGBPalette16 palette) {
    uint8_t pattern = map(edgePattern, 0, 255, 0, PATT_BACKWARD);

    uint8_t num_lit = value * (EDGE_LENGTH + 1) / 255;
    uint8_t lit_remainder = (value * (EDGE_LENGTH + 1)) % 255;
    CRGB faded_color = ColorFromPalette(palette, (num_lit + 1) * 255 / EDGE_LENGTH - 1, lit_remainder, LINEARBLEND);
//    faded_color %= lit_remainder;

    uint8_t edge_start = 0;
    uint8_t edge_end = EDGE_LENGTH;

    target_flash_edge(0, EDGE_LENGTH) = CRGB::Black;
    if (num_lit > 0) {
        
        if (num_lit >= EDGE_LENGTH) {
            target_flash_edge(0, EDGE_LENGTH) = CRGB::Black;
        } else if (pattern == PATT_CENTER) {
            edge_start = (EDGE_LENGTH - num_lit) / 2;
            edge_end = (EDGE_LENGTH + num_lit) / 2 - 1;
            
            if (num_lit % 2 == 0) {
                target_flash_edge[edge_start - 1] = faded_color;
            } else {
                target_flash_edge[edge_end + 1] = faded_color;
            }        
        } else if (pattern == PATT_FORWARD) {
            edge_end = num_lit - 1;
            target_flash_edge[edge_end + 1] = faded_color;
        } else if (pattern == PATT_BACKWARD) {
            edge_start = EDGE_LENGTH + 1 - num_lit;
            target_flash_edge[edge_start - 1] = faded_color;
        }

        int i = 0;
        for(CRGB & pixel : target_flash_edge(edge_start, edge_end)) { pixel = ColorFromPalette(palette, i * 255 / EDGE_LENGTH - 1, 255, LINEARBLEND); i++; } 
//        target_flash_edge(edge_start, edge_end) = color;
    }
    
    uint8_t blackout_val = map(blackout, 0, 255, 0, 100);

    if (blackout_val == 0) {
        current_flash_edge = target_flash_edge;
    } else {
        for (byte i=0; i < EDGE_LENGTH; i++) {
            fadeTowardColor(current_flash_edge[i], target_flash_edge[i], blackout_val);
        }
    }

    replicate_edge(edges, current_flash_edge);
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
