#include "fft_utils.h"

#include "pico/stdio.h"
#include "pico/malloc.h"
#include "pico/mem_ops.h"

#include <math.h>

// Logspace function returns a slice of n values which are logarithmically spread out between a and b.
// Values a and b are in logarithmic space already.
float* logspace(float a, float b, int n) {
    // At least two points are required
	if (n < 2) {
		return NULL;
	}

	// b value has to be bigger than a
	if (b < a) {
		return NULL;
	}

	// Step size
	float c = (b - a) / ((float)n-1);

	// Create and fill the slice
	float *out = (float *)malloc(n * sizeof(float));
    for (int i = 0; i < n; i++) {
		out[i] = powf(10, a+(float)i*c);
    }

	// Fix last entry to be 10^b
	out[n-1] = powf(10, b);

	return out;
}

// CalculateBands function returns a slice of frequency bands which will then be used to translate
// linear frequency space to a logarithmic one.
float* calculateBands(float minHz, float maxHz, int width) {
    float *bands = logspace(log10f(minHz), log10f(maxHz), width+1);
	bands[0] = minHz;
	bands[width] = maxHz;
	return bands;
}

// CalculateBins function translates linear range of frequency to appropriate fourier transform bin ranges
// in logarithmic space
void calculateBins(int minHz, int maxHz, int width, int sampleRate, int chunkSize, int *intbins, float *floatbins) {
    float *freqBands = calculateBands(minHz, maxHz, width);

	// intbins = (int *)malloc((width+1) * sizeof(int));
	// floatbins = (float *)malloc((width+1) * sizeof(float));
    for (int i = 0; i <= width; i++) {
        intbins[i] = roundf(1.0f * chunkSize * freqBands[i] / sampleRate);
		floatbins[i] = 1.0f * chunkSize * freqBands[i] / sampleRate;
		if (intbins[i] < 1) {
			intbins[i] = 1;
		} else if (intbins[i] > chunkSize) {
			intbins[i] = chunkSize;
		}

    }

	return;
}

// MaxFromRange function returns the highest value in a slice between the two indexes specified
float maxFromRange(int start, int end, float *slc) {
    float mx;
	for (int i = start; i < end; i++) {
		if (mx < abs(slc[i])) {
			mx = abs(slc[i]);
		}
	}
	return mx;
}

// fftToBins translates the result of Fourier transform which is in linear bins
// to bins in logarithmic space
void fftToBins(int *intbins, float *floatbins, float *fftData, uint8_t *out, int outLen) {
    float maxFromBins, logFromMax, lbin, lfrac;
	for (int i = 0; i < outLen; i++) {
		if (intbins[i+1]-intbins[i] <= 1) {
			lfrac = modff(floatbins[i],&lbin);
			maxFromBins = abs(fftData[(int)lbin]*(1-lfrac) + fftData[(int)lbin+1]*lfrac);
		} else {
			maxFromBins = maxFromRange(intbins[i], intbins[i+1], fftData);
		}

		if (maxFromBins > 0) {
			logFromMax = log10f(maxFromBins);
		}

		out[i] = (uint8_t)roundf(20 * logFromMax);
	}
}