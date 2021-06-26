#include <USBHost_t36.h>

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);

//              y  x
CRGB pad_colors[8][9];
CRGB pad_current[8][9];
CRGB pad_target[8][9];

midi_state mstates[90];
uint8_t midi_frontface = 3;
uint8_t midiSpeed = 5;

CRGBPalette16 midi_palette;
uint8_t midi_paletteCount;
uint8_t midi_palettePrevious;
uint8_t midi_paletteCurrent;

char launchpad_sysex_head[] = {0xf0, 0x00, 0x20, 0x29, 0x02, 0x18};
char launchpad_sysex_term = 0xf7;

#define MIDI_MAX_TIME 2000
#define MIDI_MIN_TIME 400
#define MIDI_SPEED_COUNT_TIMES 8
#define MIDI_HISTORY_MARKERS 20

unsigned long launchpad_last_update;

void initMidi() {
  // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
  // use too much power, Teensy at least completes USB enumeration, which
  // makes isolating the power issue easier.
  myusb.begin();
  
  midi_palettePrevious = 0;
  midi_paletteCurrent = 0;
  midi_paletteCount = sizeof(defPalettes) / sizeof(CRGBPalette16);
  midi_palette = createPalette(midi_paletteCurrent, CRGB::White);
  memset(&mstates[0], 0, sizeof(midi_state)*90);
  midiBlink(0,0,true);
  midiShuffleInit();

  for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t x = 0; x < 9; x++) {
          pad_colors[y][x] = CRGB::Black;
          pad_current[y][x] = CRGB::Black;
          pad_target[y][x] = CRGB::Black;
      }  
  }
  launchpad_last_update = 0;
  
  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleControlChange(myControlChange);
}

void readMidi() {
    // The handler functions are called when midi1 reads data.  They
    // will not be called automatically.  You must call midi1.read()
    // regularly from loop() for midi1 to actually read incoming
    // data and run the handler functions as messages arrive.
    myusb.Task();
    midi1.read();
//    Serial.println("Midi read start");

    uint8_t flashFrontMaxHeld = 0, flashFrontMaxEffect = 0;
    uint8_t flashBackMaxHeld = 0, flashBackMaxEffect = 0;
    clearFlashMap();
    for (uint8_t column = 1; column < 10; column++) {
        for (uint8_t row = 8; row > 0; row--) {
            uint8_t ii = row*10 + column;
            
            if (mstates[ii].note_held) {
                if (column == 4) {
                    flashFrontMaxHeld = row;
                    break;
                } else if (column == 5){
                    flashBackMaxHeld = row;
                    break;
                }
            }
            
            if (mstates[ii].effect_state) {
//                Serial.printf("Midi fun start: %d %d %d\n", ii, column, row);
                if (column < 4 && row < 5) {
                    midiDodecaImpulseHalf(column,row);
                } else if (column < 4) {
                    midiDodecaImpulseFull(column,row);
                } else if (column == 4) {
                    flashFrontMaxEffect = row;
                } else if (column == 5) {
                    flashBackMaxEffect = row;
                } else if (column == 6) {
                    midiBlink(column, row, false);
                    break;
                } else if (column == 7) {
                    midiRandomEdgeFlash(column, row);
                    break;
                } else if (column == 8) {
                    midiRandomEdgeFlashHistory(column, row);
                } else if (column == 9) {
                    midiPaletteSelect(row);
                    mstates[ii].effect_state = false;
                }
//                Serial.printf("Midi fun exit : %d %d %d\n", ii, column, row);
            }
        }  
    }

    if (flashFrontMaxHeld > 0) {
        midiFaceFlash(4,flashFrontMaxHeld,true);
    } else if (flashFrontMaxEffect > 0) {
        midiFaceFlash(4,flashFrontMaxEffect,true);
    }

    if (flashBackMaxHeld > 0) {
        midiFaceFlash(5,flashBackMaxHeld,false);
    } else if (flashBackMaxEffect > 0) {
        midiFaceFlash(5,flashBackMaxEffect,false);
    }

//    unsigned long tm = micros();
    launchpad_update();
//    Serial.println(micros() - tm);
//    Serial.println("Midi read end");
}

void myNoteOn(byte channel, byte note, byte velocity) {
    // When a USB device with multiple virtual cables is used,
    // midi1.getCable() can be used to read which of the virtual
    // MIDI cables received this message
    mstates[note].note_pressed = true;
    mstates[note].note_held = true;
    mstates[note].effect_state = true;
}

void myNoteOff(byte channel, byte note, byte velocity) {
    mstates[note].note_pressed = false;
    mstates[note].note_held = false;
}

void myControlChange(byte channel, byte control, byte value) {
    if (value == 127) {
        if (control == 104) {
            //Arrow Up - Speed increase
            midiSpeed = constrain(midiSpeed - 1, 0, MIDI_SPEED_COUNT_TIMES-1);
        } else if (control == 105) {
            //Arrow Down - Speed decrease
            midiSpeed = constrain(midiSpeed + 1, 0, MIDI_SPEED_COUNT_TIMES-1);
        } else if (control == 106) {
            //Arrow Left - Frontface decrease
            midi_frontface = constrain(midi_frontface - 1, 0, FACES_COUNT-1);
        } else if (control == 107) {
            //Arrow Right - Frontface increase
            midi_frontface = constrain(midi_frontface + 1, 0, FACES_COUNT-1);
        }
    }
}

void launchpad_light_update_pad(uint8_t x, uint8_t y, CRGB clr) {
    clr /= 4;
    if (pad_current[y-1][x-1] == clr) {
        return;
    }
    if (!clr) {
        pad_target[y-1][x-1] = CRGB::Black;
        return;
    }
    CRGB diff( abs(pad_current[y-1][x-1].r - clr.r), abs(pad_current[y-1][x-1].g - clr.g), abs(pad_current[y-1][x-1].b - clr.b));
    if (pad_current[y-1][x-1] && diff.getLuma() < 5) {
        return;
    }
    
    pad_target[y-1][x-1] = clr;
}

#define LAUNCHPAD_UPDATE_COUNT 3
#define LAUNCHPAD_UPDATE_MICROS 1500

void launchpad_update() {
    unsigned long ftime = micros();
    if (ftime - launchpad_last_update < LAUNCHPAD_UPDATE_MICROS) { return; }
    launchpad_last_update = ftime;
    
    uint8_t xs[LAUNCHPAD_UPDATE_COUNT], ys[LAUNCHPAD_UPDATE_COUNT];
    uint8_t count = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 9; j++) {
            if (pad_current[i][j] != pad_target[i][j] && count < LAUNCHPAD_UPDATE_COUNT) {
                ys[count] = i;
                xs[count] = j;
                count++;
                pad_current[i][j] = pad_target[i][j];
            }
        }
    }
    if (count == 0) { return; }
    
    // set colors to update
    uint8_t sysex_message[sizeof(launchpad_sysex_head) + sizeof(launchpad_sysex_term) + 1 + 4*LAUNCHPAD_UPDATE_COUNT];
    char message_length = sizeof(sysex_message);
    
    memcpy(&sysex_message[0], launchpad_sysex_head, sizeof(launchpad_sysex_head));
    sysex_message[sizeof(launchpad_sysex_head)] = 0x0b;

    for (int i = 0; i < count; i++) {
        CRGB clr = pad_target[ys[i]][xs[i]];
        sysex_message[sizeof(launchpad_sysex_head) + i*4 + 1] = (ys[i]+1)*10 + xs[i]+1;
        sysex_message[sizeof(launchpad_sysex_head) + i*4 + 2] = clr.r;
        sysex_message[sizeof(launchpad_sysex_head) + i*4 + 3] = clr.g;
        sysex_message[sizeof(launchpad_sysex_head) + i*4 + 4] = clr.b;
    }
    message_length -= (LAUNCHPAD_UPDATE_COUNT - count) * 4;
    sysex_message[message_length - 1] = launchpad_sysex_term;

    midi1.sendSysEx(message_length, &sysex_message[0], true);
}

void midiPaletteSelect(uint8_t y) {
    uint8_t tmp;
    switch (y) {
      case 1:
        midi_palette = createPalette(0, CRGB::White);
        break;
      case 2:
        midi_palette = createPalette(0, CRGB::Blue);
        break;
      case 3:
        midi_palette = createPalette(0, CRGB::Green);
        break;
      case 4:
        midi_palette = createPalette(0, CRGB::Red);
        break;
      case 5:
        midi_palettePrevious = midi_paletteCurrent;
        midi_paletteCurrent = (midi_paletteCurrent - 1) % midi_paletteCount;
        midi_palette = createPalette(midi_paletteCurrent, CRGB::White);
        break;
      case 6:
        midi_palettePrevious = midi_paletteCurrent;
        midi_paletteCurrent = (midi_paletteCurrent + 1) % midi_paletteCount;
        midi_palette = createPalette(midi_paletteCurrent, CRGB::White);
        break;
      case 7:
        tmp = midi_paletteCurrent;
        midi_paletteCurrent = midi_palettePrevious;
        midi_palettePrevious = tmp;
        midi_palette = createPalette(midi_paletteCurrent, CRGB::White);
        break;
      case 8:
        midi_palettePrevious = midi_paletteCurrent;
        midi_paletteCurrent = random8(midi_paletteCount)+1;
        midi_palette = createPalette(midi_paletteCurrent, CRGB::White);
        break;
    }
    // set colors to the column
    for (int i = 0; i < 8; i++) {
        launchpad_light_update_pad(9, i+1, ColorFromPalette(midi_palette, 255 * i / 7, 0xff, LINEARBLEND));
    }
}

CircularBuffer<midi_history,MIDI_HISTORY_MARKERS> midiHalfMarkers[FACES_COUNT];
#define MIDI_HISTORY_HALF 5*EDGE_LENGTH/2
//midi_history midiHalfMarkers[FACES_COUNT][MIDI_HISTORY_MARKERS];

void midiDodecaImpulseHalf(uint8_t x, uint8_t y) {
    unsigned long ftime = millis();
    uint8_t note = y*10 + x;
//    Serial.printf("XAddress: %p value: %d, YAddress: %p value: %d\n", xaddr, *xaddr, yaddr, *yaddr);

    uint8_t face = (midi_frontface + ((y-1)*3 + x-1)) % FACES_COUNT;
    if (mstates[note].note_pressed) {
        mstates[note].note_pressed = false;
        midiHalfMarkers[face].unshift(midi_history{ftime, ftime});
//        memmove(&midiHalfMarkers[face][1], &midiHalfMarkers[face][0], (MIDI_HISTORY_MARKERS-1)*sizeof(midi_history));
//        midiHalfMarkers[face][0].s = ftime;
    }
    if (mstates[note].note_held) {
        unsigned long tmp = midiHalfMarkers[face][0].s;
        midiHalfMarkers[face].shift();
        midiHalfMarkers[face].unshift(midi_history{tmp, ftime});
//        midiHalfMarkers[face][0].e = ftime;
    }

    //Using time markers of the last presses of the notes to generate a history map
    unsigned long timeSlice = map(midiSpeed, 0, MIDI_SPEED_COUNT_TIMES-1, MIDI_MIN_TIME, MIDI_MAX_TIME) / 2;
    unsigned long timeWindow = timeSlice / (MIDI_HISTORY_HALF);
    //Update the timeSlice to equal the x number of timeWindows
    timeSlice = timeWindow * MIDI_HISTORY_HALF;

    byte marker = 0;
    
    uint8_t history[MIDI_HISTORY_HALF];
    memset(&history, 0, sizeof(uint8_t) * MIDI_HISTORY_HALF);
    
    for (marker = 0; marker < MIDI_HISTORY_MARKERS && ftime - timeSlice < midiHalfMarkers[face][marker].e; marker++) {
        int endWindow = (ftime - midiHalfMarkers[face][marker].e) / timeWindow;
        int startWindow = (ftime - midiHalfMarkers[face][marker].s) / timeWindow;
        
//        Serial.printf("ftime - timeSlice: %d, midiHalfMarkers[face][marker].e: %d\n", ftime - timeSlice, midiHalfMarkers[face][marker].e);
//        Serial.printf("Loop start window range %d - %d, marker: %d\n", endWindow, startWindow, (int)marker);
        
        if (startWindow == endWindow) {
            history[startWindow] += midiHalfMarkers[face][marker].e - midiHalfMarkers[face][marker].s;
        } else {
            history[endWindow] += midiHalfMarkers[face][marker].e - (ftime - (endWindow+1)*timeWindow);
            if (startWindow < MIDI_HISTORY_HALF) {
                history[startWindow] += (ftime - startWindow*timeWindow) - midiHalfMarkers[face][marker].s;
            } else { 
                startWindow = MIDI_HISTORY_HALF;
            }
            for (int i = endWindow+1; i < startWindow; i++) {
                history[i] = timeWindow;
            }
        }
    }
    
    //If no markers make up the history then there's no future leds to update
    if (marker == 0 && history[0] == 0) { 
        mstates[note].effect_state = false;
        launchpad_light_update_pad(x, y, CRGB::Black);
        return;
    } else {
        launchpad_light_update_pad(x, y, ColorFromPalette(midi_palette, 255 * (ftime - midiHalfMarkers[face][0].e) / timeSlice, 255, LINEARBLEND));
    }

    //Update leds based on the history
    for (int i = 0; i < MIDI_HISTORY_HALF; i++) {
        history[i] = map(history[i], 0, timeWindow, 0, 0xff);
    }
    
    uint8_t face_list_num[5];
    get_all_edges_from_face(face, &face_list_num[0]);

    for (int edge_list_index = 0; edge_list_index < 3; edge_list_index++) {
        uint8_t flip=0;
        for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
            
            int led_num_start = 0;
            int8_t led_dir = 0;
            int8_t edge_num = *edge;
            if (edge_list_index == 2 && flip % 2 == 0) { edge_num = 0-edge_num; }// invert the directionality on the half of the ring edges
            flip++;
            
            if (edge_num > 0) {
                led_num_start = EDGE_LENGTH * (edge_num - 1);
                led_dir = 1;
            } else if (edge_num < 0) {
                led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                led_dir = -1;
            }
            
            for (uint8_t i = 0; i < EDGE_LENGTH; i++) {
                if (i == 10 && edge_list_index == 2) { break; }
 
                int historyIndex = EDGE_LENGTH*edge_list_index+i;
                if (history[historyIndex] > 0) {
                    leds[led_num_start + led_dir*i] = ColorFromPalette(midi_palette, 255 * historyIndex * 2 / 5 / EDGE_LENGTH, history[historyIndex], LINEARBLEND); 
                }
            }
        }
    }
}

CircularBuffer<midi_history,MIDI_HISTORY_MARKERS> midiFullMarkers[FACES_COUNT];
#define MIDI_HISTORY_FULL 5*EDGE_LENGTH

void midiDodecaImpulseFull(uint8_t x, uint8_t y) {
    unsigned long ftime = millis();
    uint8_t note = y*10 + x;

    uint8_t face = (midi_frontface + ((y-1)*3 + x-1)) % FACES_COUNT;
    if (mstates[note].note_pressed) {
        mstates[note].note_pressed = false;
        midiFullMarkers[face].unshift(midi_history{ftime, ftime});
    }
    if (mstates[note].note_held) {
        unsigned long tmp = midiFullMarkers[face][0].s;
        midiFullMarkers[face].shift();
        midiFullMarkers[face].unshift(midi_history{tmp, ftime});
    }

    //Using time markers of the last presses of the notes to generate a history map
    unsigned long timeSlice = map(midiSpeed, 0, MIDI_SPEED_COUNT_TIMES-1, MIDI_MIN_TIME, MIDI_MAX_TIME);
    unsigned long timeWindow = timeSlice / (MIDI_HISTORY_FULL);
    //Update the timeSlice to equal the x number of timeWindows
    timeSlice = timeWindow * MIDI_HISTORY_FULL;
    
    byte marker = 0;
    
    uint8_t history[MIDI_HISTORY_FULL];
    memset(&history, 0, sizeof(uint8_t) * MIDI_HISTORY_FULL);
    
    for (marker = 0; marker < MIDI_HISTORY_MARKERS && ftime - timeSlice < midiFullMarkers[face][marker].e; marker++) {
        int endWindow = (ftime - midiFullMarkers[face][marker].e) / timeWindow;
        int startWindow = (ftime - midiFullMarkers[face][marker].s) / timeWindow;
        
        if (startWindow == endWindow) {
            history[startWindow] += midiFullMarkers[face][marker].e - midiFullMarkers[face][marker].s;
        } else {
            history[endWindow] += midiFullMarkers[face][marker].e - (ftime - (endWindow+1)*timeWindow);
            if (startWindow < MIDI_HISTORY_FULL) {
                history[startWindow] += (ftime - startWindow*timeWindow) - midiFullMarkers[face][marker].s;
            } else { 
                startWindow = MIDI_HISTORY_FULL;
            }
            for (int i = endWindow+1; i < startWindow; i++) {
                history[i] = timeWindow;
            }
        }
    }
    //If no markers make up the history then there's no future leds to update
    if (marker == 0 && history[0] == 0) { 
        mstates[note].effect_state = false;
        launchpad_light_update_pad(x, y, CRGB::Black);
        return;
    } else {
        launchpad_light_update_pad(x, y, ColorFromPalette(midi_palette, 255 * (ftime - midiHalfMarkers[face][0].e) / timeSlice, 255, LINEARBLEND));
    }

    //Update leds based on the history
    for (int i = 0; i < MIDI_HISTORY_FULL; i++) {
        history[i] = map(history[i], 0, timeWindow, 0, 0xff);
    }
    
    uint8_t face_list_num[5];
    get_all_edges_from_face(face, &face_list_num[0]);

    for (int edge_list_index = 0; edge_list_index < 5; edge_list_index++) {
        uint8_t flip=0;
        for (int8_t *edge = comboEdgeGroups[face_list_num[edge_list_index]]; *edge != 0; edge++) {
            
            int led_num_start = 0;
            int8_t led_dir = 0;
            int8_t edge_num = *edge;
            if (edge_list_index == 2 && flip % 2 == 0) { edge_num = 0-edge_num; }// invert the directionality on the half of the ring edges
            flip++;
            if (edge_list_index == 3) { edge_num = 0-edge_num; }// invert the directionality on the opposite sparks part
            
            if (edge_num > 0) {
                led_num_start = EDGE_LENGTH * (edge_num - 1);
                led_dir = 1;
            } else if (edge_num < 0) {
                led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                led_dir = -1;
            }
            
            for (int i = 0; i < EDGE_LENGTH; i++) {

                int historyIndex = EDGE_LENGTH*edge_list_index+i;
                if (history[historyIndex] > 0) {
                    leds[led_num_start + led_dir*i] = ColorFromPalette(midi_palette, 255 * historyIndex / 5 / EDGE_LENGTH, history[historyIndex], LINEARBLEND); 
                }
            }
        }
    }
}


const uint16_t MIDI_FILL_LEDS       = EDGE_LENGTH * 3 / 2;
const uint16_t MIDI_MAX_FILL_FACE   = EDGE_LENGTH / 2 * 256;
const uint16_t MIDI_MAX_FILL_SPARKS = MIDI_FILL_LEDS * 256;
uint16_t fillRates[] = {5000, 6000, 9000, 12000};
uint16_t midiCurFill[2]; //Array of two for front and back

void midiFaceFlash(uint8_t x, uint8_t y, bool front) {
    uint8_t ii = 10*y + x;
    uint8_t indx = 0;
    indx = (front) ? 1 : 0;
    
    uint8_t fillrateindex = y - 1;
    if (y > 4) {
        fillrateindex -= 4;
    }
    
    if (mstates[ii].note_held) {
        if (y < 5) {
            nblendU16TowardU16(midiCurFill[indx], MIDI_MAX_FILL_FACE, fillRates[fillrateindex]);
        } else {
            nblendU16TowardU16(midiCurFill[indx], MIDI_MAX_FILL_SPARKS, fillRates[fillrateindex]);
        }
    } else {
        nblendU16TowardU16(midiCurFill[indx], 0, 15000);
    }
    
    if (midiCurFill[indx] == 0) {
        mstates[ii].effect_state = false;
        return;
    }

    uint8_t num_lit = midiCurFill[indx] >> 8;
    uint8_t lit_remainder = midiCurFill[indx] & 0xff;

    //Get front face and sparks
    uint8_t frontface = midi_frontface;
    if (!front) {
        frontface = (frontface + FACES_COUNT / 2) % FACES_COUNT;
    }
    
    //Front Face
    for (int8_t *edge = comboEdgeGroups[frontface]; *edge != 0; edge++) {
        uint16_t led_num_mid = EDGE_LENGTH * (abs(*edge) - 1) + EDGE_LENGTH/2;

        for (int i = 0; i < EDGE_LENGTH/2 && i <= num_lit; i++) {
            if (i == num_lit) {
                if (lit_remainder > 0) {
                    leds[led_num_mid + i] = ColorFromPalette(midi_palette, 0xff * i / (MIDI_FILL_LEDS), lit_remainder, LINEARBLEND);
                    leds[led_num_mid - i - 1] = ColorFromPalette(midi_palette, 0xff * i / (MIDI_FILL_LEDS), lit_remainder, LINEARBLEND);
                }
            } else {
                leds[led_num_mid + i] = ColorFromPalette(midi_palette, 0xff * i / (MIDI_FILL_LEDS), 0xff, LINEARBLEND);
                leds[led_num_mid - i - 1] = ColorFromPalette(midi_palette, 0xff * i / (MIDI_FILL_LEDS), 0xff, LINEARBLEND);
            }
        }
    }

    //Sparks
    for (int8_t *edge = comboEdgeGroups[frontface + FACES_COUNT]; *edge != 0; edge++) {
        int led_num_start = 0;
        int8_t led_dir = 0;
        int8_t edge_num = *edge;
        
        if (edge_num > 0) {
            led_num_start = EDGE_LENGTH * (edge_num - 1);
            led_dir = 1;
        } else if (edge_num < 0) {
            led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
            led_dir = -1;
        }
        
        uint16_t ii = EDGE_LENGTH / 2;
        for (int i = 0; i < EDGE_LENGTH && ii <= num_lit; i++) {
            if (ii == num_lit) {
                if (ii <= MIDI_FILL_LEDS && lit_remainder > 0) {
                    leds[led_num_start + led_dir*i] = ColorFromPalette(midi_palette, 0xff * ii / (MIDI_FILL_LEDS), lit_remainder, LINEARBLEND);
                }
            } else {
                leds[led_num_start + led_dir*i] = ColorFromPalette(midi_palette, 0xff * ii / (MIDI_FILL_LEDS), 0xff, LINEARBLEND);
            }
            ii++;
        }
    }

    // set colors to the column
    uint8_t pad_lit;
    
    if (midiCurFill[indx] < MIDI_MAX_FILL_FACE) {
        pad_lit = map(midiCurFill[indx], 0, MIDI_MAX_FILL_FACE, 0, 0x4);
    } else {
        pad_lit = map(midiCurFill[indx], MIDI_MAX_FILL_FACE, MIDI_MAX_FILL_SPARKS, 0, 0x4) + 0x4;
    }
    
    for (uint8_t i = 0; i < 8; i++) {
        if (i < pad_lit) {
            launchpad_light_update_pad(x, i+1, ColorFromPalette(midi_palette, 255 * i / 7 , 0xff, LINEARBLEND));
        } else {
            launchpad_light_update_pad(x, i+1, CRGB::Black);
        }
    }
}

#define MIDI_BLINK_NTH_SMALL 8
#define MIDI_BLINK_NTH_LARGE 6
const uint16_t MIDI_BLINK_COUNT_SMALL = NUM_LEDS / MIDI_BLINK_NTH_SMALL;
const uint16_t MIDI_BLINK_COUNT_LARGE = NUM_LEDS / MIDI_BLINK_NTH_LARGE;
midi_blink midiBlinksSmall[MIDI_BLINK_COUNT_SMALL];
midi_blink midiBlinksLarge[MIDI_BLINK_COUNT_LARGE];
unsigned long midiBlinkTick;

//Think about changing this mode so that it linearly scales the nth parameter
//The lowest pad means that every nth led will be blinking, while the top one means all
//This may mean that it would be possible to hold only a single array that contains all of the LEDs
//This would also simplify the code
void midiBlink(uint8_t x, uint8_t y, bool initBlink) {
    uint8_t ii = 10*y + x;
    if (initBlink) {
        for (int i = 0; i < MIDI_BLINK_COUNT_SMALL; i++ ) {
            midiBlinksSmall[i].index = i * MIDI_BLINK_NTH_SMALL + random8(MIDI_BLINK_NTH_SMALL);
            midiBlinksSmall[i].value = random16();
            midiBlinksSmall[i].pIndex = random8();
        }
        for (int i = 0; i < MIDI_BLINK_COUNT_LARGE; i++ ) {
            midiBlinksLarge[i].index = i * MIDI_BLINK_NTH_LARGE + random8(MIDI_BLINK_NTH_LARGE);
            midiBlinksLarge[i].value = random16();
            midiBlinksLarge[i].pIndex = random8();
        }
        midiBlinkTick = millis();
        return;
    }
    if (!mstates[ii].note_held) {
        mstates[ii].effect_state = false;
        for (int i = 0; i < 8; i++) {
            launchpad_light_update_pad(x, i+1, CRGB::Black);
        }
        return;
    }

    unsigned long ftime = millis();
    unsigned long delta = ftime - midiBlinkTick;
    uint16_t valueDelta = delta << (9-midiSpeed);

    if (y < 5) {
        for (int i = 0; i < MIDI_BLINK_COUNT_SMALL; i++ ) {
            if (0xffff - midiBlinksSmall[i].value < valueDelta) {
                midiBlinksSmall[i].index = i * MIDI_BLINK_NTH_SMALL + random8(MIDI_BLINK_NTH_SMALL);
                midiBlinksSmall[i].pIndex = random8();
            }
            midiBlinksSmall[i].value -= valueDelta;
            leds[midiBlinksSmall[i].index] = ColorFromPalette(midi_palette, midiBlinksSmall[i].pIndex, midiBlinksSmall[i].value >> 8, LINEARBLEND);
        }
        for (int i = 0; i < y; i++) {
            launchpad_light_update_pad(x, i+1, ColorFromPalette(midi_palette, midiBlinksSmall[i].pIndex, midiBlinksSmall[i].value >> 8, LINEARBLEND));
        }
    } else {
        for (int i = 0; i < MIDI_BLINK_COUNT_LARGE; i++ ) {
            if (0xffff - midiBlinksLarge[i].value < valueDelta) {
                midiBlinksLarge[i].index = i * MIDI_BLINK_NTH_LARGE + random8(MIDI_BLINK_NTH_LARGE);
                midiBlinksLarge[i].pIndex = random8();
            }
            midiBlinksLarge[i].value -= valueDelta;
            leds[midiBlinksLarge[i].index] = ColorFromPalette(midi_palette, midiBlinksLarge[i].pIndex, midiBlinksLarge[i].value >> 8, LINEARBLEND);
        }
        for (int i = 0; i < y; i++) {
            launchpad_light_update_pad(x, i+1, ColorFromPalette(midi_palette, midiBlinksLarge[i].pIndex, midiBlinksLarge[i].value >> 8, LINEARBLEND));
        }
    }

    midiBlinkTick = ftime;
}

//midi_blink midiBlinks[NUM_LEDS];
//unsigned long midiBlinkTick;
//uint8_t midiBlinkLastY;
//
//void midiBlink(uint8_t x, uint8_t y, bool initBlink) {
//    uint8_t ii = 10*y + x;
//    if (initBlink) {
//        for (int i = 0; i < NUM_LEDS; i++ ) {
//            midiBlinks[i].index = i;
//            midiBlinks[i].value = random16();
//            midiBlinks[i].pIndex = random8();
//        }
//        midiBlinkLastY = 1;
//        midiBlinkTick = millis();
//        return;
//    }
//    if (!mstates[ii].note_held) {
//        mstates[ii].effect_state = false;
//        return;
//    }
//
//    unsigned long ftime = millis();
//    unsigned long delta = ftime - midiBlinkTick;
//    uint16_t valueDelta = delta << (9-midiSpeed);
//
//    uint8_t nth = 9 - y;
//    if (midiBlinkLastY != y) {
//        for (int i = 0; i < NUM_LEDS; i += nth ) {
//            midiBlinks[i].index = i * nth + random8(nth); 
//        }
//        midiBlinkLastY = y;
//        Serial.println("Index refresh");
//    }
//    
//    for (int i = 0; i < NUM_LEDS; i += nth ) {
//        if (0xffff - midiBlinks[i].value < valueDelta) {
//            midiBlinks[i].index = i * nth + random8(nth);
//            midiBlinks[i].pIndex = random8();
//        }
//        midiBlinks[i].value -= valueDelta;
//        leds[midiBlinks[i].index] = ColorFromPalette(midi_palette, midiBlinks[i].pIndex, midiBlinks[i].value >> 8, LINEARBLEND);
//    }
//
//    midiBlinkTick = ftime;
//}

uint8_t midiRandShuffle[NUM_EDGES];
uint8_t midiRandShuffleEdges[3*8];

//Fix so that the random edges are still selected while note is held in
void midiRandomEdgeFlash(uint8_t x, uint8_t y) { 
    uint8_t ii = 10*y + x;
    if (!mstates[ii].note_held) {
        mstates[ii].effect_state = false;
        launchpad_light_update_pad(x, y, CRGB::Black);
        return;
    }
    
    uint8_t nEdge = 3*y;
    int8_t led_dir = 1;
    uint8_t midiRandFirstEdgeIndex = 0;
    
    if (mstates[ii].note_pressed) {
        mstates[ii].note_pressed = false;
        midiShuffle(3*NUM_EDGES);
        midiRandFirstEdgeIndex = random8(NUM_EDGES-nEdge);
        for (int i = 0; i < nEdge; i++) {
            midiRandShuffleEdges[i] = midiRandShuffle[midiRandFirstEdgeIndex+i];
        }
    }
    
    for (int i = 0; i < nEdge; i++) {
        uint16_t led_num_start = 0;
        uint8_t edge_num = midiRandShuffleEdges[i];
        
        if (led_dir > 0) {
            led_num_start = EDGE_LENGTH * (edge_num - 1);
        } else {
            led_num_start = EDGE_LENGTH * edge_num - 1;
        }
        
        for (int j = 0; j < EDGE_LENGTH; j++) {
            leds[led_num_start + led_dir*j] = ColorFromPalette(midi_palette, 0xff * j / EDGE_LENGTH, 0xff, LINEARBLEND);
        }
      
        led_dir = (-1)*led_dir;
    }
    launchpad_light_update_pad(x, y, ColorFromPalette(midi_palette, 0x7f, 0xff, LINEARBLEND));
}

#define MIDI_HISTORY_FLASH_MARKERS 10
CircularBuffer<midi_flash_history,MIDI_HISTORY_FLASH_MARKERS> midiFlashMarkers[8];
CRGBArray<EDGE_LENGTH> midiFlashEdge;
bool midiFlashMap[NUM_EDGES];

void clearFlashMap() {
    for (int i = 0; i < NUM_EDGES; i++) {
        midiFlashMap[i] = false;
    }
}

//Fix so that the random edges are still selected while note is held in
void midiRandomEdgeFlashHistory(uint8_t x, uint8_t y) {
    unsigned long ftime = millis();
    uint8_t ii = 10*y + x;
    uint8_t nEdge = 3*y;
    
    if (mstates[ii].note_pressed) {
        mstates[ii].note_pressed = false;
        midiShuffle(3*NUM_EDGES);
        
        midi_flash_history tmp;
        memset(&tmp.edges, 0, NUM_EDGES);
        uint8_t firstEdgeIndex = random8(NUM_EDGES-nEdge);
        
        for (int i = 0; i < nEdge; i++) {
            tmp.edges[i] = midiRandShuffle[firstEdgeIndex + i];
            if (i % 2 == 1) {
                tmp.edges[i] *= (-1);
            }
        }
        tmp.ended = false;
        tmp.s = ftime;
        tmp.e = 0;

        midiFlashMarkers[y-1].unshift(tmp);
    } else if (!mstates[ii].note_held && !midiFlashMarkers[y-1][0].ended) {
        midi_flash_history tmp = midiFlashMarkers[y-1].shift();
        tmp.ended = true;
        tmp.e = ftime;
        midiFlashMarkers[y-1].unshift(tmp);
    }

    //Using time markers of the last presses of the notes to generate a history map
    unsigned long timeSlice = map(midiSpeed, 0, MIDI_SPEED_COUNT_TIMES-1, MIDI_MIN_TIME, MIDI_MAX_TIME) / 5;
    unsigned long timeWindow = timeSlice / (EDGE_LENGTH);
    //Update the timeSlice to equal the x number of timeWindows
    timeSlice = timeWindow * EDGE_LENGTH;

    byte marker = 0;
    bool any_proceeded = false;
    uint8_t history[EDGE_LENGTH];
        
    for (marker = 0; marker < MIDI_HISTORY_FLASH_MARKERS && (!midiFlashMarkers[y-1][marker].ended || ftime - timeSlice < midiFlashMarkers[y-1][marker].e); marker++) {
        //Check if any of the edges would be impacted by this run
        bool proceed = false;
        for (int i = 0; i < 3*8+1; i++) {
            int8_t edge = midiFlashMarkers[y-1][marker].edges[i];
            if (edge == 0) {
                break;
            }
            if (!midiFlashMap[abs(edge)]) {
                proceed = true;
                break;
            }
        }
        if (!proceed) { continue; }
        any_proceeded = true;
        if (marker == 0 && proceed) {
            if (midiFlashMarkers[y-1][marker].ended) {
                launchpad_light_update_pad(x, y, ColorFromPalette(midi_palette, 255 * (ftime - midiFlashMarkers[y-1][marker].e) / timeSlice, 0xff, LINEARBLEND));
            } else {
                launchpad_light_update_pad(x, y, ColorFromPalette(midi_palette, 0x00, 0xff, LINEARBLEND));
            }
        }
      
        memset(&history, 0, sizeof(uint8_t) * EDGE_LENGTH);
        
        int endWindow = 0;
        if (midiFlashMarkers[y-1][marker].ended) {
            endWindow = (ftime - midiFlashMarkers[y-1][marker].e) / timeWindow;
        }
        int startWindow = (ftime - midiFlashMarkers[y-1][marker].s) / timeWindow;
        
        if (startWindow == 0 && startWindow == endWindow) {
            history[0] = ftime - midiFlashMarkers[y-1][marker].s;
        } else if (startWindow == endWindow) {
            history[startWindow] = midiFlashMarkers[y-1][marker].e - midiFlashMarkers[y-1][marker].s;
        } else {
            if (midiFlashMarkers[y-1][marker].ended) {
                history[endWindow] = midiFlashMarkers[y-1][marker].e - (ftime - (endWindow+1)*timeWindow);
            } else {
                history[endWindow] = timeWindow;
            }
            
            if (startWindow < EDGE_LENGTH) {
                history[startWindow] = (ftime - startWindow*timeWindow) - midiFlashMarkers[y-1][marker].s;
            } else { 
                startWindow = EDGE_LENGTH;
            }
            for (int i = endWindow+1; i < startWindow; i++) {
                history[i] = timeWindow;
            }
        }
        
        //Update leds based on the history
        for (int i = 0; i < EDGE_LENGTH; i++) {
            history[i] = map(history[i], 0, timeWindow, 0, 0xff);
        }

        for (uint8_t i = 0; i < EDGE_LENGTH; i++) {
            midiFlashEdge[i] = ColorFromPalette(midi_palette, 255 * i / (EDGE_LENGTH - 1), history[i], LINEARBLEND); 
        }

        for (int i = 0; i < 3*8+1; i++) {
            int8_t edge = midiFlashMarkers[y-1][marker].edges[i];
            if (edge == 0) {
                break;
            }
            
            if (!midiFlashMap[abs(edge)]) {
                midiFlashMap[abs(edge)] = true;

                int8_t led_dir = 0;
                int8_t edge_num = edge;
                uint16_t led_num_start = 0;
                
                if (edge_num > 0) {
                    led_num_start = EDGE_LENGTH * (edge_num - 1);
                    led_dir = 1;
                } else if (edge_num < 0) {
                    led_num_start = EDGE_LENGTH * (0 - edge_num) - 1;
                    led_dir = -1;
                }

                for (uint8_t i = 0; i < EDGE_LENGTH; i++) {
                    leds[led_num_start + led_dir*i] = midiFlashEdge[i];
                }
            }
        }
    }
    
    //If no markers make up the history then there's no future leds to update
    if (!any_proceeded) { 
        mstates[ii].effect_state = false;
        launchpad_light_update_pad(x, y, CRGB::Black);
    }
}

void midiShuffle(uint8_t times) {
    for (int i = 0; i < times; i++) {
        uint8_t old = random8(NUM_EDGES);
        uint8_t nw = random8(NUM_EDGES);
        uint8_t tmp = midiRandShuffle[old];
        midiRandShuffle[old] = midiRandShuffle[nw];
        midiRandShuffle[nw] = tmp;
    }
}

void midiShuffleInit() {
    for (int i=0; i < NUM_EDGES; i++) {
        midiRandShuffle[i] = i+1;
    }
    midiShuffle(3*NUM_EDGES);
}
