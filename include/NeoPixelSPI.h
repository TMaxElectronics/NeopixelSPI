#ifndef NEOPIXEL_INC
#define NEOPIXEL_INC

#include "SPI.h"

typedef enum{RGB, RGBW} LedColorType_t;

typedef struct{
    SPI_HANDLE * spiHandle;
    uint32_t arrayLength;
    LedColorType_t colorType;
    
    uint8_t * outputData;
    uint32_t dataLength;
    uint32_t bitsPerPixel;
} NP_Handle_t;

void NP_update(NP_Handle_t * handle);
void NP_setPixel(NP_Handle_t * handle, uint32_t i, uint32_t r, uint32_t g, uint32_t b, uint32_t w);
NP_Handle_t * NP_init(SPI_HANDLE * spiHandle, uint32_t arrayLength, LedColorType_t colorType);

#endif
