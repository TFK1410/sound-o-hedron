#ifndef FFT_UTILS_H
#define FFT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

// Logspace function returns a slice of n values which are logarithmically spread out between a and b.
// Values a and b are in logarithmic space already.
float* logspace(float a, float b, int n);

// MaxFromRange function returns the highest value in a slice between the two indexes specified
float maxFromRange(int start, int end, float *slc);

// CalculateBands function returns a slice of frequency bands which will then be used to translate
// linear frequency space to a logarithmic one.
float* calculateBands(float minHz, float maxHz, int width);

// CalculateBins function translates linear range of frequency to appropriate fourier transform bin ranges
// in logarithmic space
void calculateBins(int minHz, int maxHz, int width, int sampleRate, int chunkSize, int *intbins, float *floatbins);

// fftToBins translates the result of Fourier transform which is in linear bins
// to bins in logarithmic space
void fftToBins(int *intbins, float *floatbins, float *fftData, uint8_t *out, int outLen);

#ifdef __cplusplus
}
#endif
#endif