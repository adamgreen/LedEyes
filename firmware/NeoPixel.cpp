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
#include <assert.h>
#include <mbed.h>
#include "GPDMA.h"
#include "NeoPixel.h"


// This class utilizes DMA based SPI hardware to send data to NeoPixel LEDs.
// It was only coded to work on the LPC1768.
#ifndef TARGET_LPC176X
    #error("This NeoPixel class was only coded to work on the LPC1768.")
#endif



NeoPixel::NeoPixel(uint32_t ledCount, PinName outputPin) : SPI(outputPin, NC, NC)
{
    // Each NeoPixel data-bit should be 1.2 usec so use 12 SPI bits when running SPI at 10MHz.
    const uint32_t spiBitsPerNeoPixelBit = 12;
    const uint32_t bitsPerPixel = 24;
    // 500 * 1/10MHz = 50 usec.
    const uint32_t resetBits = 500;

    format(8, 3);
    frequency(10000000);

    m_flipCount = 0;
    m_isStarted = false;
    m_ledCount = ledCount;
    m_backBufferState = BackBufferFree;
    m_backBufferId = 0;
    m_frontBufferIds[0] = 0;
    m_frontBufferIds[0] = 0;

    // Round up byte count.
    uint32_t ledBits = ledCount * bitsPerPixel * spiBitsPerNeoPixelBit;
    // The byte count dedicated to LED output data should be an even multiple of 3 bytes.
    assert ( (ledBits % 8) == 0 );
    m_ledBytes = ledBits / 8;
    m_packetSize = m_ledBytes + (resetBits + 7) / 8;

    // Place buffers used by DMA code in separate RAM bank to optimize performance.
    m_pFrontBuffers[0] = (uint8_t*)dmaHeap0Alloc(m_packetSize);
    m_pFrontBuffers[1] = (uint8_t*)dmaHeap1Alloc(m_packetSize);
    m_pBackBuffer = (uint8_t*)malloc(m_packetSize);

    setConstantBitsInBuffers();

    // Setup GPDMA module.
    enableGpdmaPower();
    enableGpdmaInLittleEndianMode();

    // Add this DMA handler to the linked list of DMA handlers.
    m_dmaHandler.handler = __spiTransmitInterruptHandler;
    m_dmaHandler.pContext = (void*)this;
    addDmaInterruptHandler(&m_dmaHandler);

    // Initialize the DMA mem copy callback structure;
    m_dmaMemCopyCallback.handler = __memCopyCompleteHandler;
    m_dmaMemCopyCallback.pContext = (void*)this;
}

void NeoPixel::setConstantBitsInBuffers()
{
    setConstantBitsInBuffer(m_pBackBuffer);
    memcpy(m_pFrontBuffers[0], m_pBackBuffer, m_packetSize);
    memcpy(m_pFrontBuffers[1], m_pBackBuffer, m_packetSize);
}

void NeoPixel::setConstantBitsInBuffer(uint8_t* pBuffer)
{
    // Each NeoPixel bit will be represented in 12 SPI bits.
    // The format of the 12 SPI bits will be:
    //    1111xxxx0000
    //    xxxx will be 0000 if NeoPixel bit is 0.
    //    xxxx will be 1110 if NeoPixel bit is 1.
    // Two NeoPixel bits will therefore be stored in 3 SPI bytes.
    //    1111xxxx 00001111 xxxx0000
    uint8_t* pEnd = pBuffer + m_ledBytes;
    while (pBuffer + 2 < pEnd)
    {
        // Fill in buffer 3 bytes at a time.
        *pBuffer++ = 0xF0;
        *pBuffer++ = 0x0F;
        *pBuffer++ = 0x00;
    }
    // The byte count dedicated to LED output data should be an even multiple of 3 bytes.
    assert ( pBuffer == pEnd );

    // Set frame reset bits to 0.
    memset(pBuffer, 0, m_packetSize - m_ledBytes);
}

NeoPixel::~NeoPixel()
{
    if (m_isStarted)
    {
        freeDmaChannel(m_channelTx);
    }
    removeDmaInterruptHandler(&m_dmaHandler);
    uninitDmaMemCopy();
}

void NeoPixel::start()
{
    if (m_isStarted)
    {
        return;
    }

    // Allocate DMA channel for transmitting.
    m_channelTx = allocateDmaChannel(GPDMA_CHANNEL_LOW);
    m_pChannelTx = dmaChannelFromIndex(m_channelTx);
    m_sspTx = (_spi.spi == (LPC_SSP_TypeDef*)SPI_1) ? DMA_PERIPHERAL_SSP1_TX : DMA_PERIPHERAL_SSP0_TX;

    // Clear error and terminal complete interrupts for transmit channel.
    uint32_t channelMask = 1 << m_channelTx;
    LPC_GPDMA->DMACIntTCClear = channelMask;
    LPC_GPDMA->DMACIntErrClr  = channelMask;

    // Prepare transmit channel DMA circular linked list to use 2 front buffers.
    m_dmaListItems[0].DMACCxSrcAddr  = (uint32_t)m_pFrontBuffers[0];
    m_dmaListItems[0].DMACCxDestAddr = (uint32_t)&_spi.spi->DR;
    m_dmaListItems[0].DMACCxLLI      = (uint32_t)&m_dmaListItems[1];
    m_dmaListItems[0].DMACCxControl  = DMACCxCONTROL_I | DMACCxCONTROL_SI |
                     (DMACCxCONTROL_BURSTSIZE_4 << DMACCxCONTROL_SBSIZE_SHIFT) |
                     (DMACCxCONTROL_BURSTSIZE_4 << DMACCxCONTROL_DBSIZE_SHIFT) |
                     (m_packetSize & DMACCxCONTROL_TRANSFER_SIZE_MASK);

    m_dmaListItems[1].DMACCxSrcAddr  = (uint32_t)m_pFrontBuffers[1];
    m_dmaListItems[1].DMACCxDestAddr = (uint32_t)&_spi.spi->DR;
    m_dmaListItems[1].DMACCxLLI      = (uint32_t)&m_dmaListItems[0];
    m_dmaListItems[1].DMACCxControl  = DMACCxCONTROL_I | DMACCxCONTROL_SI |
                     (DMACCxCONTROL_BURSTSIZE_4 << DMACCxCONTROL_SBSIZE_SHIFT) |
                     (DMACCxCONTROL_BURSTSIZE_4 << DMACCxCONTROL_DBSIZE_SHIFT) |
                     (m_packetSize & DMACCxCONTROL_TRANSFER_SIZE_MASK);

    m_pChannelTx->DMACCSrcAddr  = m_dmaListItems[0].DMACCxSrcAddr;
    m_pChannelTx->DMACCDestAddr = m_dmaListItems[0].DMACCxDestAddr;
    m_pChannelTx->DMACCLLI      = m_dmaListItems[0].DMACCxLLI;
    m_pChannelTx->DMACCControl  = m_dmaListItems[0].DMACCxControl;

    // Enable transmit channel.
    m_pChannelTx->DMACCConfig = DMACCxCONFIG_ENABLE |
                   (m_sspTx << DMACCxCONFIG_DEST_PERIPHERAL_SHIFT) |
                   DMACCxCONFIG_TRANSFER_TYPE_M2P |
                   DMACCxCONFIG_IE |
                   DMACCxCONFIG_ITC;

    // Turn on DMA transmit requests in SSP.
    _spi.spi->DMACR = (1 << 1);

    m_isStarted = true;
}

void NeoPixel::set(const RGBData* pPixels, size_t pixelCount)
{
    assert ( pixelCount == m_ledCount );

    waitForFreeBackBuffer();

    // Emit bits into the now free back buffer.
    m_pEmitBuffer = m_pBackBuffer;
    for (uint32_t i = 0 ; i < m_ledCount ; i++)
    {
        RGBData led = *pPixels++;

        emitByte(led.red);
        emitByte(led.green);
        emitByte(led.blue);
    }

    // Let the DMA interrupt handler know that the back buffer is now ready to be copied into the next free
    // front buffer.
    m_backBufferId++;
    m_backBufferState = BackBufferReadyToCopy;

    m_setCount++;
}

void NeoPixel::waitForFreeBackBuffer()
{
    // Might hang forever if DMA operations haven't been started yet so just return.
    if (!m_isStarted)
        return;

    while (m_backBufferState != BackBufferFree)
    {
        // Don't hit the memory bus too hard querying m_backBufferState while other DMA operations are running against
        // the main SRAM bank.
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

void NeoPixel::emitByte(uint8_t byte)
{
    // Each NeoPixel bit will be represented in 12 SPI bits.
    // The format of the 12 SPI bits will be:
    //    1111xxxx0000
    //    xxxx will be 0000 if NeoPixel bit is 0.
    //    xxxx will be 1110 if NeoPixel bit is 1.
    // Two NeoPixel bits will therefore be stored in 3 SPI bytes.
    //    1111xxxx 00001111 xxxx0000

    // Bit 7
    *m_pEmitBuffer &= 0xF0;
    if (byte & 0x80)
        *m_pEmitBuffer |= 0x0E;
    m_pEmitBuffer += 2;
    // Bit 6
    *m_pEmitBuffer &= 0x0F;
    if (byte & 0x40)
        *m_pEmitBuffer |= 0xE0;
    m_pEmitBuffer++;
    // Bit 5
    *m_pEmitBuffer &= 0xF0;
    if (byte & 0x20)
        *m_pEmitBuffer |= 0x0E;
    m_pEmitBuffer += 2;
    // Bit 4
    *m_pEmitBuffer &= 0x0F;
    if (byte & 0x10)
        *m_pEmitBuffer |= 0xE0;
    m_pEmitBuffer++;
    // Bit 3
    *m_pEmitBuffer &= 0xF0;
    if (byte & 0x08)
        *m_pEmitBuffer |= 0x0E;
    m_pEmitBuffer += 2;
    // Bit 2
    *m_pEmitBuffer &= 0x0F;
    if (byte & 0x04)
        *m_pEmitBuffer |= 0xE0;
    m_pEmitBuffer++;
    // Bit 1
    *m_pEmitBuffer &= 0xF0;
    if (byte & 0x02)
        *m_pEmitBuffer |= 0x0E;
    m_pEmitBuffer += 2;
    // Bit 0
    *m_pEmitBuffer &= 0x0F;
    if (byte & 0x01)
        *m_pEmitBuffer |= 0xE0;
    m_pEmitBuffer++;
}

uint32_t NeoPixel::__spiTransmitInterruptHandler(void* pContext, uint32_t dmaInterruptStatus)
{
    NeoPixel* pThis = (NeoPixel*)pContext;

    return pThis->spiTransmitInterruptHandler(dmaInterruptStatus);
}

uint32_t NeoPixel::spiTransmitInterruptHandler(uint32_t dmaInterruptStatus)
{
    uint32_t txChannelMask = 1 << m_channelTx;

    if ((dmaInterruptStatus & txChannelMask) == 0)
    {
        return 0;
    }

    // Handle flipping from one front buffer to the other.
    // Determine which of the front buffers was just rendered and which one is just starting to render.
    uint32_t bufferJustSent = m_flipCount & 1;
    uint32_t bufferToSendNext = !bufferJustSent;

    if (m_backBufferState == BackBufferReadyToCopy)
    {
        // There is a new back buffer to copy into the front buffer.
        m_backBufferState = BackBufferCopying;
        int usedDma = dmaMemCopy(m_pFrontBuffers[bufferJustSent], m_pBackBuffer, m_packetSize, &m_dmaMemCopyCallback);
        assert ( usedDma );
        (void)usedDma;
        m_frontBufferIds[bufferJustSent] = m_backBufferId;
    }
    else if (m_frontBufferIds[bufferJustSent] != m_frontBufferIds[bufferToSendNext])
    {
        // Copy newer frame buffer into this older/stale one.
        int usedDma = dmaMemCopy(m_pFrontBuffers[bufferJustSent],
                                 m_pFrontBuffers[bufferToSendNext],
                                 m_packetSize,
                                 &m_dmaMemCopyCallback);
        assert ( usedDma );
        (void)usedDma;
        m_frontBufferIds[bufferJustSent] = m_frontBufferIds[bufferToSendNext];
    }

    m_flipCount++;

    // Flag that we have handled this interrupt.
    LPC_GPDMA->DMACIntTCClear = txChannelMask;
    return txChannelMask;
}

void NeoPixel::__memCopyCompleteHandler(void* pContext)
{
    NeoPixel* pThis = (NeoPixel*)pContext;

    pThis->memCopyCompleteHandler();
}

void NeoPixel::memCopyCompleteHandler()
{
    // Let the client app know if the back buffer was just freed for it to reuse for next frame.
    if (m_backBufferState == BackBufferCopying)
    {
        m_backBufferState = BackBufferFree;
    }
}
