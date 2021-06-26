#define DMX_MODE_COUNT 3 //modes repeat 3 times

struct mode_data {
    uint8_t params[3];
    uint8_t curve;
    CRGB color;
};

struct dmx_data {
    uint8_t modes[DMX_MODE_COUNT];
    uint8_t front_face;
    mode_data mdata[DMX_MODE_COUNT];
};

struct midi_state {
    bool note_pressed;
    bool note_held;
    bool effect_state;
};

struct midi_history {
    unsigned long s;
    unsigned long e;
};

struct midi_blink {
    uint16_t index;
    uint16_t value;
    uint8_t pIndex;
};

struct midi_flash_history {
    unsigned long s;
    unsigned long e;
    bool ended;
    int8_t edges[3*8+1];
};
