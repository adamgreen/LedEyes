/* Copyright (C) 2016  Adam Green (https://github.com/adamgreen)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef NEO_PIXEL_H_
#define NEO_PIXEL_H_

#include <mbed.h>
#include "Pixel.h"
#include "GPDMA.h"


class NeoPixel : public SPI
{
public:
    NeoPixel(uint32_t ledCount, PinName outputPin);
    ~NeoPixel();

    void     start();
    void     set(const RGBData* pPixels, size_t pixelCount);

    // Number of times set() method was called.
    uint32_t getSetCount()
    {
        return m_setCount;
    }
    // Number of times that front buffers were rendered to NeoPixel strip.
    uint32_t getFlipCount()
    {
        return m_flipCount;
    }

protected:
    void setConstantBitsInBuffers();
    void setConstantBitsInBuffer(uint8_t* pBuffer);
    void waitForFreeBackBuffer();
    void emitByte(uint8_t byte);

    static uint32_t __spiTransmitInterruptHandler(void* pContext, uint32_t dmaInterruptStatus);
    uint32_t        spiTransmitInterruptHandler(uint32_t dmaInterruptStatus);
    static void     __memCopyCompleteHandler(void* pContext);
    void            memCopyCompleteHandler();

    enum BackBufferState
    {
        BackBufferFree,
        BackBufferCopying,
        BackBufferReadyToCopy
    };

    uint8_t*                    m_pFrontBuffers[2];
    uint8_t*                    m_pBackBuffer;
    uint8_t*                    m_pEmitBuffer;
    LPC_GPDMACH_TypeDef*        m_pChannelTx;
    DmaInterruptHandler         m_dmaHandler;
    DmaMemCopyCallback          m_dmaMemCopyCallback;
    DmaLinkedListItem           m_dmaListItems[2];
    uint32_t                    m_channelTx;
    uint32_t                    m_sspTx;
    uint32_t                    m_ledCount;
    uint32_t                    m_ledBytes;
    uint32_t                    m_packetSize;
    uint32_t                    m_setCount;
    volatile uint32_t           m_flipCount;
    volatile uint32_t           m_backBufferId;
    volatile uint32_t           m_frontBufferIds[2];
    volatile BackBufferState    m_backBufferState;
    bool                        m_isStarted;
    uint8_t                     m_dummyRead;
};

#endif // NEO_PIXEL_H_
