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
