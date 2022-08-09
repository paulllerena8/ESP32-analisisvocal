#include <Arduino.h>

// 16bit, monoral, 44100Hz,  linear PCM
void CreateWavHeader(byte* header, int waveDataSize);  // Tamaño de cabecera de 44
