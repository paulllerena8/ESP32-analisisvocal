#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#define SAMPLE_RATE (48000)
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 22
#define PIN_I2S_DIN 34
#define PIN_I2S_DOUT 25

/// @parameter MODE : I2S_MODE_RX or I2S_MODE_TX
/// @parameter BPS : I2S_BITS_PER_SAMPLE_16BIT or I2S_BITS_PER_SAMPLE_32BIT
void I2S_Init(i2s_mode_t MODE, i2s_bits_per_sample_t BPS);

/// I2S_Read() for I2S_MODE_RX
/// @parameter data: pointer to buffer
/// @parameter numData: buffer size
/// @return Number of bytes read
int I2S_Read(char* data, int numData);

/// I2S_Write() for I2S_MODE_TX
/// @param data: pointer to buffer
/// @param numData: buffer size
void I2S_Write(char* data, int numData);
