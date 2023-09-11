
#include <xc.h>
#include <stdint.h>

#include "FreeRTOS.h"

#include "NeoPixelSPI.h"
#include "SPI.h"
#include "System.h"

NP_Handle_t * NP_init(SPI_HANDLE * spiHandle, uint32_t arrayLength, LedColorType_t colorType){
    NP_Handle_t * ret = pvPortMalloc(sizeof(NP_Handle_t));
    
    ret->spiHandle = spiHandle;
    ret->arrayLength = arrayLength;
    ret->colorType = colorType;
    
    //allocate output data buffer. Size needs to accommodate three bits per bit in the color data => size = colorsPerLED*8*3/8 = colorsPerLED * 3
    switch(colorType){
        case RGB:
            ret->outputData = pvPortMalloc(3*arrayLength);
            ret->dataLength = 3*arrayLength;
            break;
            
        case RGBW:
            ret->outputData = pvPortMalloc(4*arrayLength);
            ret->dataLength = 4*arrayLength;
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
}

void NP_setPixel(NP_Handle_t * handle, uint32_t i, uint32_t r, uint32_t g, uint32_t b, uint32_t w){
    if(handle == NULL) return;
    
    //update output data
    
    //get position within the array
    uint32_t bitCount = 0;
    switch(handle->colorType){
        case RGB:
            bitCount = 24;
            break;
            
        case RGBW:
            bitCount = 32;
            break;
            
        default:
            //invalid color config selected, return
            return;
    }
    uint32_t startBit = bitCount * i; 
   
    uint32_t currByte = startBit / 8;
    uint32_t currBit = startBit % 8;
    
    for(uint32_t bitInColor = 0; bitInColor < bitCount; bitInColor++){
        //get current bit
        
        uint32_t bitValue = 0;
        //which color is being written right now?
        switch(bitInColor/8){
            case 0: //red
                bitValue = (r & SYS_BITMASK[bitInColor % 8]) > 0;
                break;
            case 1: //green
                bitValue = (g & SYS_BITMASK[bitInColor % 8]) > 0;
                break;
            case 2: //blue
                bitValue = (b & SYS_BITMASK[bitInColor % 8]) > 0;
                break;
            case 3: //white
                bitValue = (w & SYS_BITMASK[bitInColor % 8]) > 0;
                break;
        }
        
        
        //write current bit to output array
        
        //clear bit first
        handle->outputData[currByte] &= ~SYS_BITMASK[bitValue];
        //set bit if data == 1
        if(bitValue) handle->outputData[currByte] |= SYS_BITMASK[bitValue];
        
        
        //continue to next bit
        if(++currBit >= 8){
            currBit = 0;
            currByte ++;
        }
    }
}

void NP_update(NP_Handle_t * handle){
    SPI_sendBytes(handle->spiHandle, handle->outputData, handle->dataLength, 1, 0, NULL, NULL);
}