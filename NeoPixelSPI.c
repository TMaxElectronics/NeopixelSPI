
#include <xc.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "NeoPixelSPI.h"
#include "SPI.h"
#include "System.h"

static uint32_t ledCodes[] = {0b110, 0b100};

static void setOutputBit(NP_Handle_t * handle, uint32_t bitIndex, uint32_t value){
    //move bitIndex to position in output data array
    bitIndex = bitIndex * 3 + 1;
    //convert to bit position in output array
    uint32_t currByte = (bitIndex) >> 3;    // = / 8
    uint32_t currBit = (bitIndex) & 0b111;  // = % 8
    
    handle->outputData[currByte] &= ~SYS_BITMASK[7-currBit];
    if(value) handle->outputData[currByte] |= SYS_BITMASK[7-currBit];
}

static void initOutputData(NP_Handle_t * handle){
    if(handle == NULL) return;
    uint32_t currBit = 0;
    uint32_t currByte = 0;
    
    while(currByte < handle->dataLength){
        handle->outputData[currByte] |= SYS_BITMASK[7-currBit];
        //continue to next bit
        if(++currBit >= 8){
            currBit = 0;
            currByte ++;
        }
        handle->outputData[currByte] &= ~SYS_BITMASK[7-currBit];
        if(++currBit >= 8){
            currBit = 0;
            currByte ++;
        }
        handle->outputData[currByte] &= ~SYS_BITMASK[7-currBit];
        if(++currBit >= 8){
            currBit = 0;
            currByte ++;
        }
    }
}

NP_Handle_t * NP_init(SPI_HANDLE * spiHandle, uint32_t arrayLength, LedColorType_t colorType){
    NP_Handle_t * ret = pvPortMalloc(sizeof(NP_Handle_t));
    
    ret->spiHandle = spiHandle;
    ret->arrayLength = arrayLength;
    ret->colorType = colorType;
    
    //allocate output data buffer. Size needs to accommodate three bits per bit in the color data => size = colorsPerLED*8*3/8 = colorsPerLED * 3
    switch(colorType){
        case RGB:
            ret->outputData = pvPortMalloc(3*3*arrayLength);
            ret->dataLength = 3*3*arrayLength;
            ret->bitsPerPixel = 3*8;
            break;
            
        case RGBW:
            ret->outputData = pvPortMalloc(3*4*arrayLength);
            ret->dataLength = 3*4*arrayLength;
            ret->bitsPerPixel = 4*8;
            break;
            
        default:
            //invalid color config selected, return NULL
            vPortFree(ret);
            configASSERT(0);
            return NULL;
    }
    
    //clock frequency must be adjusted so one bit is exactly 400ns wide => 1/400ns = 2.5MHz
    SPI_setCLKFreq(spiHandle, 2500000);
    
    //disable input pin
    SPI_setCustomPinConfig(spiHandle, 0, 1);
    
    SPI_setDMAEnabled(spiHandle, 1);
    
    initOutputData(ret);
    
    return ret;
}

void NP_setPixel(NP_Handle_t * handle, uint32_t i, uint32_t r, uint32_t g, uint32_t b, uint32_t w){
    if(handle == NULL) return;
    
    //update output data
    uint32_t currBit = handle->bitsPerPixel * i;
    
    for(uint32_t bitInColor = 0; bitInColor < handle->bitsPerPixel; bitInColor++){
        //get current bit
        
        uint32_t bitValue = 0;
        //which color is being written right now?
        switch(bitInColor/8){
            case 1: //red
                bitValue = (r & SYS_BITMASK[7 - (bitInColor % 8)]) > 0;
                break;
            case 0: //green
                bitValue = (g & SYS_BITMASK[7 - (bitInColor % 8)]) > 0;
                break;
            case 2: //blue
                bitValue = (b & SYS_BITMASK[7 - (bitInColor % 8)]) > 0;
                break;
            case 3: //white
                bitValue = (w & SYS_BITMASK[7 - (bitInColor % 8)]) > 0;
                break;
        }
        
        //write data into array
        setOutputBit(handle, currBit+bitInColor, bitValue);
    }
}

void NP_update(NP_Handle_t * handle){
    SPI_sendBytes(handle->spiHandle, handle->outputData, handle->dataLength, 0, 0, NULL, NULL);
}