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
#include "Animation.h"


// powf(255.0f, (float)x / 255.0);
uint8_t AnimationBase::s_powerTable[256] =
{
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
      2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
      4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,
      5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,
      8,   8,   8,   8,   8,   8,   9,   9,   9,   9,  10,  10,  10,  10,  10,  11,
     11,  11,  11,  12,  12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,  15,
     16,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,  20,  21,  21,  22,
     22,  23,  23,  24,  24,  25,  26,  26,  27,  27,  28,  29,  29,  30,  30,  31,
     32,  33,  33,  34,  35,  36,  36,  37,  38,  39,  40,  41,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  51,  52,  53,  54,  55,  56,  58,  59,  60,  62,  63,
     64,  66,  67,  69,  70,  72,  73,  75,  77,  78,  80,  82,  84,  86,  87,  89,
     91,  93,  95,  98, 100, 102, 104, 106, 109, 111, 114, 116, 119, 121, 124, 127,
    130, 132, 135, 138, 141, 144, 148, 151, 154, 158, 161, 165, 168, 172, 176, 180,
    184, 188, 192, 196, 200, 205, 209, 214, 219, 223, 228, 233, 238, 244, 249, 255
};

// log2f((float)x)/log2f(255.0f)*255.0f
uint8_t AnimationBase::s_logTable[256] =
{
      0,   0,  31,  50,  63,  74,  82,  89,  95, 101, 105, 110, 114, 118, 121, 124,
    127, 130, 133, 135, 137, 140, 142, 144, 146, 148, 149, 151, 153, 154, 156, 158,
    159, 160, 162, 163, 164, 166, 167, 168, 169, 170, 172, 173, 174, 175, 176, 177,
    178, 179, 180, 180, 181, 182, 183, 184, 185, 186, 186, 187, 188, 189, 189, 190,
    191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 198, 198, 199, 199, 200, 201,
    201, 202, 202, 203, 203, 204, 204, 205, 206, 206, 207, 207, 208, 208, 209, 209,
    210, 210, 210, 211, 211, 212, 212, 213, 213, 214, 214, 215, 215, 215, 216, 216,
    217, 217, 217, 218, 218, 219, 219, 219, 220, 220, 221, 221, 221, 222, 222, 222,
    223, 223, 223, 224, 224, 225, 225, 225, 226, 226, 226, 227, 227, 227, 228, 228,
    228, 229, 229, 229, 229, 230, 230, 230, 231, 231, 231, 232, 232, 232, 232, 233,
    233, 233, 234, 234, 234, 234, 235, 235, 235, 236, 236, 236, 236, 237, 237, 237,
    237, 238, 238, 238, 238, 239, 239, 239, 239, 240, 240, 240, 240, 241, 241, 241,
    241, 242, 242, 242, 242, 243, 243, 243, 243, 244, 244, 244, 244, 244, 245, 245,
    245, 245, 246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 248, 248, 248, 248,
    249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 252,
    252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 255,
};


static unsigned int posRand();


AnimationBase::AnimationBase()
{
    m_pStart = NULL;
    m_pEnd = NULL;
    m_pCurr = NULL;
    m_pInterpolating = NULL;
    m_pRgbPixels = NULL;
    m_pHsvPrev = NULL;
    m_pHsvNext = NULL;
    m_pixelCount = 0;
    m_lastRenderTime = 0xFFFFFFFF;
    m_dirty = false;
    m_timer.start();
}

void AnimationBase::setKeyFrames(const AnimationKeyFrame* pFrames, size_t frameCount)
{
    m_pStart = pFrames;
    m_pEnd = pFrames + frameCount;
    m_pCurr = pFrames;
    m_pInterpolating = NULL;
    m_timer.reset();
    m_lastRenderTime = 0xFFFFFFFF;
    m_dirty = true;
}

void AnimationBase::updatePixels(NeoPixel& ledControl)
{
    if (m_timer.read_ms() > m_pCurr->millisecondsBeforeNextFrame)
    {
        m_pCurr++;
        if (m_pCurr >= m_pEnd)
        {
            m_pCurr = m_pStart;
        }
        m_timer.reset();
        m_dirty = true;
    }

    if (m_pCurr->interpolateBetweenFrames)
    {
        updatePixelsInterpolated(ledControl);
    }
    else
    {
        updatePixelsNonInterpolated(ledControl);
    }
}

void AnimationBase::updatePixelsNonInterpolated(NeoPixel& ledControl)
{
    if (m_dirty)
    {
        ledControl.set(m_pCurr->pPixels, m_pixelCount);
        m_dirty = false;
    }
}

void AnimationBase::updatePixelsInterpolated(NeoPixel& ledControl)
{
    if (m_pCurr != m_pInterpolating)
    {
        // Want to interpolate between HSV values so convert the two frames to that colour space at the beginning of
        // an interpolation sequence.
        convertRgbPixelsToHsv(m_pHsvPrev, m_pCurr->pPixels, m_pixelCount);

        const AnimationKeyFrame* pNextFrame = m_pCurr + 1;
        if (pNextFrame >= m_pEnd)
        {
            pNextFrame = m_pStart;
        }
        convertRgbPixelsToHsv(m_pHsvNext, pNextFrame->pPixels, m_pixelCount);

        m_lastRenderTime = 0xFFFFFFFF;
        m_pInterpolating = m_pCurr;
    }

    // Don't render the interpolation more than once per millisecond.
    int32_t currTime = m_timer.read_ms();
    if (currTime != m_lastRenderTime)
    {
        interpolateBetweenKeyFrames(currTime, m_pCurr->millisecondsBeforeNextFrame);
        ledControl.set(m_pRgbPixels, m_pixelCount);
        m_lastRenderTime = currTime;
    }
}

void AnimationBase::convertRgbPixelsToHsv(HSVData* pHsvDest, const RGBData* pRgbSrc, size_t pixelCount)
{
    while (pixelCount--)
    {
        rgbToInterpolatableHsv(pHsvDest++, pRgbSrc++);
    }
}

void AnimationBase::rgbToInterpolatableHsv(HSVData* pHsvDest, const RGBData* pRgbSrc)
{
    rgbToHsv(pHsvDest, pRgbSrc);

    // An exponential curve is used during interpolation for brightness. This inverses that so that the beginning
    // and end of the interpolation will result in the same RGB values as in the keyframe.
    pHsvDest->value = s_logTable[pHsvDest->value];
}

void AnimationBase::interpolateBetweenKeyFrames(int32_t currTime, int32_t totalTime)
{
    const HSVData* pPrev = m_pHsvPrev;
    const HSVData* pNext = m_pHsvNext;
    RGBData* pRgb = m_pRgbPixels;

    for (size_t i = 0 ; i < m_pixelCount ; i++)
    {
        interpolateHsvToRgb(pRgb++, pPrev++, pNext++, currTime, totalTime);
    }
}

void AnimationBase::interpolateHsvToRgb(RGBData* pRgbDest, const HSVData* pHsvStart, const HSVData* pHsvStop,
                                    int32_t curr, int32_t total)
{
    HSVData interpolated;

    int32_t huePrev = pHsvStart->hue;
    int32_t hueNext = pHsvStop->hue;
    interpolated.hue = huePrev + ((hueNext - huePrev) * curr) / total;

    int32_t saturationPrev = pHsvStart->saturation;
    int32_t saturationNext = pHsvStop->saturation;
    interpolated.saturation = saturationPrev + ((saturationNext - saturationPrev) * curr) / total;

    // Use an exponential curve for brightness to make the interpolation perception smoother to the human eye.
    int32_t valuePrev = pHsvStart->value;
    int32_t valueNext = pHsvStop->value;
    int32_t newValue = (valuePrev + ((valueNext - valuePrev) * curr) / total);
    interpolated.value = s_powerTable[newValue];

    hsvToRgb(pRgbDest, &interpolated);
}




TwinkleAnimationBase::TwinkleAnimationBase()
{
    m_pProperties = NULL;
    m_pRgbPixels = NULL;
    m_pHsvPixels = NULL;
    m_pTwinkleInfo = NULL;
    m_pixelCount = 0;
    m_lastUpdate = -1;
    m_timer.start();
}

void TwinkleAnimationBase::setProperties(const TwinkleProperties* pProperties)
{
    m_pProperties = pProperties;
    memset(m_pRgbPixels, 0, sizeof(*m_pRgbPixels) * m_pixelCount);
    memset(m_pHsvPixels, 0, sizeof(*m_pHsvPixels) * m_pixelCount);
    memset(m_pTwinkleInfo, 0, sizeof(*m_pTwinkleInfo) * m_pixelCount);

    m_lastUpdate = -1;
    m_timer.reset();
}

void TwinkleAnimationBase::updatePixels(NeoPixel& ledControl)
{
    int32_t currTime = m_timer.read_ms();
    if (m_lastUpdate == currTime)
    {
        // Only do any work once each millisecond.
        return;
    }
    m_lastUpdate = currTime;

    // Animate twinkles already in progress.
    HSVData* pHsvPixel = m_pHsvPixels;
    HSVData* pEnd = m_pHsvPixels + m_pixelCount;
    RGBData* pRgbPixel = m_pRgbPixels;
    PixelTwinkleInfo* pInfo = m_pTwinkleInfo;
    while (pHsvPixel < pEnd)
    {
        twinklePixel(pRgbPixel++, pHsvPixel++, pInfo++, currTime);
    }

    // Randomly start twinkling pixels.
    if (posRand() % m_pProperties->probability != 0)
    {
        // Don't need to start another twinkle at this time.
        ledControl.set(m_pRgbPixels, m_pixelCount);
        return;
    }

    // Pick the pixel to twinkle.
    int pixelToTwinkle = posRand() % m_pixelCount;
    pInfo = &m_pTwinkleInfo[pixelToTwinkle];
    if (pInfo->lifetime != 0)
    {
        // Don't bother since it is already in the process of twinkling.
        ledControl.set(m_pRgbPixels, m_pixelCount);
        return;
    }

    // Configure this pixel for twinkling.
    uint32_t lifetimeDelta = m_pProperties->lifetimeMax - m_pProperties->lifetimeMin;
    pInfo->lifetime = m_pProperties->lifetimeMin + (lifetimeDelta ? posRand() % lifetimeDelta : 0);
    pInfo->startTime = currTime;
    pInfo->isGettingBrighter = true;

    pHsvPixel = &m_pHsvPixels[pixelToTwinkle];
    uint8_t hueDelta = m_pProperties->hueMax - m_pProperties->hueMin;
    uint8_t saturationDelta = m_pProperties->saturationMax - m_pProperties->saturationMin;
    uint8_t valueDelta = m_pProperties->valueMax - m_pProperties->valueMin;

    pHsvPixel->hue = m_pProperties->hueMin + (hueDelta ? posRand() % hueDelta : 0);
    pHsvPixel->saturation = m_pProperties->saturationMin + (saturationDelta ? posRand() % saturationDelta : 0);
    pHsvPixel->value = m_pProperties->valueMin + (valueDelta ? posRand() % valueDelta : 0);

    // Set RGB pixel value to match starting colour.
    HSVData hsvStart = *pHsvPixel;
    hsvStart.value = 8;
    pRgbPixel = &m_pRgbPixels[pixelToTwinkle];
    hsvToRgb(pRgbPixel, &hsvStart);

    ledControl.set(m_pRgbPixels, m_pixelCount);
}

static unsigned int posRand()
{
    return (unsigned int)rand();
}

void TwinkleAnimationBase::twinklePixel(RGBData* pRgbDest,
                                    const HSVData* pHsv,
                                    PixelTwinkleInfo* pInfo,
                                    uint32_t currTime)
{
    if (pInfo->lifetime == 0)
    {
        // This pixel isn't twinkling so just return.
        return;
    }

    uint32_t deltaTime = currTime - pInfo->startTime;
    if (pInfo->isGettingBrighter)
    {
        if (deltaTime > pInfo->lifetime)
        {
            // Time to start dimming down.
            pInfo->isGettingBrighter = false;
            pInfo->startTime = currTime;
            deltaTime = 0;
        }
        else
        {
            HSVData hsvStart = *pHsv;
            HSVData hsvStop = *pHsv;
            hsvStart.value = 8;
            AnimationBase::interpolateHsvToRgb(pRgbDest, &hsvStart, &hsvStop, deltaTime, pInfo->lifetime);
        }
    }

    // Check explicitly for opposite direction since the direction of fade might have been reversed inside of
    // above conditional when it has already reached maximum brightness.
    if (!pInfo->isGettingBrighter)
    {
        if (deltaTime > pInfo->lifetime)
        {
            // The twinkle is complete so flag it as being so and turn LED off.
            pInfo->lifetime = 0;
            *pRgbDest = { 0, 0, 0 };
            return;
        }
        HSVData hsvStart = *pHsv;
        HSVData hsvStop = *pHsv;
        hsvStop.value = 8;
        AnimationBase::interpolateHsvToRgb(pRgbDest, &hsvStart, &hsvStop, deltaTime, pInfo->lifetime);
    }
}




FlickerAnimationBase::FlickerAnimationBase()
{
    m_pProperties = NULL;
    m_pRgbPixels = NULL;
    m_pHsvPixels = NULL;
    m_pFlickerInfo = NULL;
    m_pixelCount = 0;
    m_lastUpdate = -1;
    m_timer.start();
}

void FlickerAnimationBase::setProperties(const FlickerProperties* pProperties)
{
    m_pProperties = pProperties;
    HSVData hsvColour;

    // Fill in starting HSV colour for all LEDs to be desired RGB colour but with brightness modified to maximum value.
    rgbToHsv(&hsvColour, &pProperties->baseRGBColour);
    hsvColour.value = pProperties->brightnessMax;
    createRepeatingPixelPattern((RGBData*)m_pHsvPixels, m_pixelCount, (const RGBData*)&hsvColour, 1);

    memset(m_pRgbPixels, 0, sizeof(*m_pRgbPixels) * m_pixelCount);
    memset(m_pFlickerInfo, 0, sizeof(*m_pFlickerInfo) * m_pixelCount);

    m_lastUpdate = -1;
    m_timer.reset();
}

void FlickerAnimationBase::updatePixels(NeoPixel& ledControl)
{
    int32_t currTime = m_timer.read_ms();
    if (m_lastUpdate == currTime)
    {
        // Only do any work once each millisecond.
        return;
    }
    m_lastUpdate = currTime;

    // Run through all LEDs and flicker them.
    HSVData* pHsvPixel = m_pHsvPixels;
    HSVData* pEnd = m_pHsvPixels + m_pixelCount;
    RGBData* pRgbPixel = m_pRgbPixels;
    PixelFlickerInfo* pInfo = m_pFlickerInfo;
    while (pHsvPixel < pEnd)
    {
        updatePixel(pRgbPixel++, pHsvPixel++, pInfo++, currTime);
    }

    ledControl.set(m_pRgbPixels, m_pixelCount);
}

void FlickerAnimationBase::updatePixel(RGBData* pRgbDest,
                                       const HSVData* pHsv,
                                       PixelFlickerInfo* pInfo,
                                       uint32_t currTime)
{
    uint32_t deltaTime = currTime - pInfo->startTime;
    if (deltaTime > pInfo->time)
    {
        // Done interpolating to new brightness level. Flag that a new level should be selected.
        pInfo->time = 0;
    }
    else
    {
        AnimationBase::interpolateHsvToRgb(pRgbDest, &pInfo->hsvStart, &pInfo->hsvStop, deltaTime, pInfo->time);
    }

    if (pInfo->time == 0)
    {
        // Pick a new brightnes level to interpolate towards.
        uint32_t timeDelta = m_pProperties->timeMax - m_pProperties->timeMin;
        pInfo->time = m_pProperties->timeMin + (timeDelta ? posRand() % timeDelta : 0);
        pInfo->startTime = currTime;

        pInfo->hsvStart = *pHsv;
        pInfo->hsvStop = *pHsv;

        // The brightness level should have a 2x chance of being brightest setting compared to other values.
        uint32_t brightnessDelta = m_pProperties->brightnessMax - m_pProperties->brightnessMin;
        uint32_t randValue = posRand() % (m_pProperties->stayBrightFactor * brightnessDelta);
        pInfo->hsvStop.value = (randValue > brightnessDelta) ? m_pProperties->brightnessMax :
                                                               m_pProperties->brightnessMin + randValue;

        return;
    }
}