/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/i2c_slave.h>
#include <pico/time.h>
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "DmxInput.h"
#include "kiss_fftr.h"
#include "fft_utils.h"

// Channel 0 is GPIO26
#define CAPTURE_CHANNEL 0
#define NUM_CHANNELS 2
#define CAPTURE_DEPTH 1500
#define SAMPLE_RATE 44100
#define ADC_CLOCK_RATE 48000000

#define FFT_RATE 40 // per sec
#define FFT_INTERVAL_US 1000000 / FFT_RATE
#define FFT_SMOOTH_RATE 60 // per sec
#define FFT_SMOOTH_INTERVAL_US 1000000 / FFT_SMOOTH_RATE
#define FFT_SMOOTH_CURVE 0.05

#define FREQ_MIN 45
#define FREQ_MAX 20000
#define FFT_OUTPUT 16

#define DMX_START_CHANNEL 13
#define DMX_COUNT_CHANNEL 25
#define DMX_PIN 0

// Switching between the audio and DMX mode is handled via a single GPIO pin where the Teensy is the writer and Pico is the reader
// LOW - Audio mode; HIGH - DMX mode
#define MODE_PIN 16 // pin 21
bool dmxModeOn = false;

uint8_t captureBuf[CAPTURE_DEPTH * NUM_CHANNELS];
uint8_t lbuf[CAPTURE_DEPTH];
uint8_t rbuf[CAPTURE_DEPTH];
uint8_t fftbuf[NUM_CHANNELS*FFT_OUTPUT];
float fftbufSmoothFloat[NUM_CHANNELS*FFT_OUTPUT];
uint8_t fftbufSmooth[NUM_CHANNELS*FFT_OUTPUT];

uint dmaChan;
dma_channel_config cfg;
absolute_time_t lastFFTUpdate, lastFFTSmoothUpdate;

DmxInput myDmxInput;
volatile uint8_t dmxBuffer[DMXINPUT_BUFFER_SIZE(DMX_START_CHANNEL, DMX_COUNT_CHANNEL+1)];
unsigned long dmxLastUpdate = 0;
uint8_t dmxs[DMX_COUNT_CHANNEL + 1];

static const uint I2C_SLAVE_ADDRESS = 0x01;
static const uint I2C_BAUDRATE = 100000; // 100 kHz
static const uint I2C_SLAVE_SDA_PIN = PICO_DEFAULT_I2C_SDA_PIN; // 4
static const uint I2C_SLAVE_SCL_PIN = PICO_DEFAULT_I2C_SCL_PIN; // 5

void setup_gpio();
void setup_adc();
void setup_dma();
void sample();
void split_channels();
void setup_i2c();
void setup_dmx();

void audio_fft_core();

int main() {

    stdio_init_all();

    setup_gpio();

    setup_adc();

    setup_dma();

    setup_dmx();

    multicore_launch_core1(audio_fft_core);

    // smoothing FFT loop
    while (true) {
        // if (gpio_get(MODE_PIN)) {
        //     if (!dmxModeOn) {
        //         // Turn on DMX
        //         myDmxInput.begin(DMX_PIN, DMX_START_CHANNEL, DMX_COUNT_CHANNEL);
        //         myDmxInput.read_async(dmxBuffer);
        //         dmxModeOn = true;
        //     }
        //     sleep(100);
        // } else if (!gpio_get(MODE_PIN)) {
        //     if (dmxModeOn) {
        //         // Turn off DMX
        //         myDmxInput.end();
        //         dmxModeOn = false;
        //     }
        // }
        if (gpio_get(MODE_PIN)) {
            // DMX MODE - this core sleeps
            sleep_ms(1000);
        } else if (!gpio_get(MODE_PIN)) {
            // AUDIO MODE - this core smooths fft results
            lastFFTSmoothUpdate = get_absolute_time();


            for (int i = 0; i < NUM_CHANNELS * FFT_OUTPUT; i++) {
                fftbufSmoothFloat[i] = FFT_SMOOTH_CURVE*fftbufSmoothFloat[i] + (1-FFT_SMOOTH_CURVE)*fftbuf[i];
                fftbufSmooth[i] = (uint8_t)fftbufSmoothFloat[i];
            }

            // Print samples to stdout so you can display them in pyplot, excel, matlab
            for (int i = 0; i < NUM_CHANNELS * FFT_OUTPUT; ++i) {
                printf("%5.1f,", fftbufSmoothFloat[i]);
            }
            printf("\n");
            
            int64_t fftSmoothTime = absolute_time_diff_us(lastFFTSmoothUpdate, get_absolute_time());
            int64_t sleepTime = FFT_SMOOTH_INTERVAL_US - fftSmoothTime;
            if (sleepTime > 0) {
                sleep_us((uint64_t)sleepTime);
            }
        }

    }
}

void fft_calc(kiss_fft_scalar *fft_in, float *fft_out_scal, kiss_fftr_cfg *cfg, uint8_t *buf) {
    kiss_fft_cpx fft_out[CAPTURE_DEPTH];
    // fill fourier transform input while subtracting DC component
    uint64_t sum = 0;
    for (int i=0;i<CAPTURE_DEPTH;i++) {sum+=buf[i];}
    float avg = (float)sum/CAPTURE_DEPTH;
    for (int i=0;i<CAPTURE_DEPTH;i++) {fft_in[i]=(float)buf[i]-avg;}
    // printf("%f\n", avg);

    // compute fast fourier transform
    kiss_fftr(*cfg , fft_in, fft_out);

    // any frequency bin over NSAMP/2 is aliased (nyquist sampling theorum)
    for (int i = 0; i < CAPTURE_DEPTH/2; i++) {
        fft_out_scal[i] = fft_out[i].r*fft_out[i].r+fft_out[i].i*fft_out[i].i;
    }
}

void audio_fft_core() {
    // calculating actual FFT loop
    kiss_fft_scalar fft_in[CAPTURE_DEPTH]; // kiss_fft_scalar is a float
    float fft_out_scal[CAPTURE_DEPTH / 2];
    kiss_fftr_cfg cfg = kiss_fftr_alloc(CAPTURE_DEPTH,false,0,0);

    int intbins[FFT_OUTPUT+1];
    float floatbins[FFT_OUTPUT+1];
    calculateBins(FREQ_MIN, FREQ_MAX, FFT_OUTPUT, SAMPLE_RATE, CAPTURE_DEPTH, intbins, floatbins);

    while (true) {
        lastFFTUpdate = get_absolute_time();

        // print precalculated bin border values
        // for (int i = 0; i <= FFT_OUTPUT; ++i) {
        //     printf("%.3f,", floatbins[i]);
        // }
        // printf("\n");

        sample();

        split_channels();

        // Print samples to stdout so you can display them in pyplot, excel, matlab
        // for (int i = 0; i < CAPTURE_DEPTH; ++i) {
        //     printf("%-3d,", rbuf[i]);
        //     if (i % 40 == 39)
        //         printf("\n");
        // }

        fft_calc(fft_in, fft_out_scal, &cfg, lbuf);

        fftToBins(intbins, floatbins, fft_out_scal, &fftbuf[0], FFT_OUTPUT);

        fft_calc(fft_in, fft_out_scal, &cfg, rbuf);

        fftToBins(intbins, floatbins, fft_out_scal, &fftbuf[FFT_OUTPUT], FFT_OUTPUT);

        // Print fft results to stdout
        // for (int i = 0; i < NUM_CHANNELS*FFT_OUTPUT; ++i) {
        //     printf("%d,", fftbuf[i]);
        // }
        // printf("\n");

        int64_t fftTime = absolute_time_diff_us(lastFFTUpdate, get_absolute_time());
        int64_t sleepTime = FFT_INTERVAL_US - fftTime;
        // printf("%d\n", sleepTime);
        if (sleepTime > 0) {
            sleep_us((uint64_t)sleepTime);
        }
    }
}

void setup_gpio() {
    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_IN);
}

void setup_adc(){
    int ch_mask=0;

    for (int ch = 0; ch < NUM_CHANNELS; ch++)
    {
        // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
        adc_gpio_init(26 + ch);
        ch_mask += 1 << ch;
    }

    adc_init();
    adc_set_round_robin(ch_mask); // Use for 2 channel measurement : ch 0 & 1 -> 0b0011 = 3
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        true     // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    // adc_set_clkdiv(0);
    // We set the clock div for the audio sampling rate
    adc_set_clkdiv(ADC_CLOCK_RATE / SAMPLE_RATE / NUM_CHANNELS);
}

void setup_dma() {
    // printf("Arming DMA\n");
    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dmaChan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(dmaChan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);
}

void sample() {
    // Set starting ADC channel 
    adc_select_input(CAPTURE_CHANNEL);

    dma_channel_configure(dmaChan, &cfg,
        captureBuf,    // dst
        &adc_hw->fifo,  // src
        CAPTURE_DEPTH,  // transfer count
        true            // start immediately
    );

    // printf("Starting capture\n");

    adc_run(true);
    dma_channel_wait_for_finish_blocking(dmaChan);

    // printf("Capture finished\n");

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    adc_run(false);
    adc_fifo_drain();
}

void split_channels() {
    // printf("Splitting channels\n");
    for(uint16_t i=0;i<CAPTURE_DEPTH*NUM_CHANNELS;i++){
        if (i%2==0) {rbuf[i/2] = captureBuf[i];}
        else {lbuf[i/2+1] = captureBuf[i];}
    }
}

void setup_dmx() {
    myDmxInput.begin(DMX_PIN, DMX_START_CHANNEL, DMX_COUNT_CHANNEL);
    myDmxInput.read_async(dmxBuffer);
}

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data
        if (gpio_get(MODE_PIN)) {
            dmxs[0] = 0;
            if (dmxLastUpdate < myDmxInput.latest_packet_timestamp()) {
                dmxLastUpdate = to_ms_since_boot(get_absolute_time());
                dmxs[0] = 255;
                // memcpy(&dmxs[1], buffer, COUNT_CHANNEL);   
                for (int i = 0; i < DMX_COUNT_CHANNEL; i++) {
                    dmxs[i+1] = dmxBuffer[i];
                }
            }
            i2c_write_raw_blocking(i2c, dmxs, DMX_COUNT_CHANNEL + 1);
        } else {
            i2c_write_raw_blocking(i2c, fftbufSmooth, FFT_OUTPUT*NUM_CHANNELS);
        }
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
        break;
    default:
        break;
    }
}

void setup_i2c() {    
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);

    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);

    i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, I2C_SLAVE_ADDRESS, &i2c_slave_handler);
}
