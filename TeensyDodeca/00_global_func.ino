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
