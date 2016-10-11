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
#ifndef ANIMATION_H_
#define ANIMATION_H_

#include <assert.h>
#include <mbed.h>
#include "NeoPixel.h"


struct AnimationKeyFrame
{
    RGBData* pPixels;
    int32_t  millisecondsBeforeNextFrame;
    bool     interpolateBetweenFrames;
};

class IPixelUpdate
{
public:
    virtual void updatePixels(NeoPixel& ledControl) = 0;
};

class AnimationBase : public IPixelUpdate
{
public:

    void setKeyFrames(const AnimationKeyFrame* pFrames, size_t frameCount);

    // IPixelUpdate methods.
    virtual void updatePixels(NeoPixel& ledControl);

    // Static methods used together to interpolate colour values.
    static void rgbToInterpolatableHsv(HSVData* pHsvDest, const RGBData* pRgbSrc);
    static void interpolateHsvToRgb(RGBData* pRgbDest, const HSVData* pHsvStart, const HSVData* pHsvStop,
                                    int32_t curr, int32_t total);
protected:
    AnimationBase();

    void updatePixelsNonInterpolated(NeoPixel& ledControl);
    void updatePixelsInterpolated(NeoPixel& ledControl);
    void convertRgbPixelsToHsv(HSVData* pHsvDest, const RGBData* pRgbSrc, size_t pixelCount);
    void interpolateBetweenKeyFrames(int32_t currTime, int32_t totalTime);

    static uint8_t           s_powerTable[256];
    static uint8_t           s_logTable[256];
    const AnimationKeyFrame* m_pStart;
    const AnimationKeyFrame* m_pEnd;
    const AnimationKeyFrame* m_pCurr;
    const AnimationKeyFrame* m_pInterpolating;
    RGBData*                 m_pRgbPixels;
    HSVData*                 m_pHsvPrev;
    HSVData*                 m_pHsvNext;
    size_t                   m_pixelCount;
    Timer                    m_timer;
    int32_t                  m_lastRenderTime;
    bool                     m_dirty;
};

template <size_t PIXEL_COUNT>
class Animation : public AnimationBase
{
public:
    Animation()
    {
        m_pRgbPixels = m_rgbPixels;
        m_pHsvPrev = m_hsvPrevPixels;
        m_pHsvNext = m_hsvNextPixels;
        m_pixelCount = PIXEL_COUNT;
    }

protected:
    RGBData m_rgbPixels[PIXEL_COUNT];
    HSVData m_hsvPrevPixels[PIXEL_COUNT];
    HSVData m_hsvNextPixels[PIXEL_COUNT];
};



struct TwinkleProperties
{
    // The twinkle should fade in and out in this number of milliseconds.
    uint32_t lifetimeMin;
    uint32_t lifetimeMax;
    // There should be a 1 in probability milliseconds chance of a twinkle starting.
    int      probability;
    // The colour of the twinkling LED should be constrained to these HSV limits.
    uint8_t  hueMin;
    uint8_t  hueMax;
    uint8_t  saturationMin;
    uint8_t  saturationMax;
    uint8_t  valueMin;
    uint8_t  valueMax;
};

class TwinkleAnimationBase : public IPixelUpdate
{
public:

    void setProperties(const TwinkleProperties* pProperties);

    // IPixelUpdate methods.
    virtual void updatePixels(NeoPixel& ledControl);

protected:
    TwinkleAnimationBase();

    struct PixelTwinkleInfo
    {
        uint32_t startTime;
        uint32_t lifetime;
        bool     isGettingBrighter;
    };
    void twinklePixel(RGBData* pRgbDest, const HSVData* pHsv, PixelTwinkleInfo* pInfo, uint32_t currTime);

    const TwinkleProperties* m_pProperties;
    RGBData*                 m_pRgbPixels;
    HSVData*                 m_pHsvPixels;
    PixelTwinkleInfo*        m_pTwinkleInfo;
    size_t                   m_pixelCount;
    Timer                    m_timer;
    int32_t                  m_lastUpdate;
};

template <size_t PIXEL_COUNT>
class TwinkleAnimation : public TwinkleAnimationBase
{
public:
    TwinkleAnimation()
    {
        m_pixelCount = PIXEL_COUNT;
        m_pRgbPixels = m_rgbPixels;
        m_pHsvPixels = m_hsvPixels;
        m_pTwinkleInfo = m_twinkleInfo;
    }

protected:
    RGBData          m_rgbPixels[PIXEL_COUNT];
    HSVData          m_hsvPixels[PIXEL_COUNT];
    PixelTwinkleInfo m_twinkleInfo[PIXEL_COUNT];
};



struct FlickerProperties
{
    // The brightness changes should fade in and out in this number of milliseconds.
    uint32_t        timeMin;
    uint32_t        timeMax;
    // How much more often should the LED just maintain maximum brightness than flickering.
    uint32_t        stayBrightFactor;
    uint8_t         brightnessMin;
    uint8_t         brightnessMax;
    RGBData         baseRGBColour;
};

class FlickerAnimationBase : public IPixelUpdate
{
public:

    void setProperties(const FlickerProperties* pProperties);

    // IPixelUpdate methods.
    virtual void updatePixels(NeoPixel& ledControl);

protected:
    FlickerAnimationBase();

    struct PixelFlickerInfo
    {
        uint32_t startTime;
        uint32_t time;
        HSVData  hsvStart;
        HSVData  hsvStop;
    };
    void updatePixel(RGBData* pRgbDest, const HSVData* pHsv, PixelFlickerInfo* pInfo, uint32_t currTime);

    const FlickerProperties* m_pProperties;
    RGBData*                 m_pRgbPixels;
    HSVData*                 m_pHsvPixels;
    PixelFlickerInfo*        m_pFlickerInfo;
    size_t                   m_pixelCount;
    Timer                    m_timer;
    int32_t                  m_lastUpdate;
};

template <size_t PIXEL_COUNT>
class FlickerAnimation : public FlickerAnimationBase
{
public:
    FlickerAnimation()
    {
        m_pixelCount = PIXEL_COUNT;
        m_pRgbPixels = m_rgbPixels;
        m_pHsvPixels = m_hsvPixels;
        m_pFlickerInfo = m_flickerInfo;
    }

protected:
    RGBData          m_rgbPixels[PIXEL_COUNT];
    HSVData          m_hsvPixels[PIXEL_COUNT];
    PixelFlickerInfo m_flickerInfo[PIXEL_COUNT];
};



static inline void createRepeatingPixelPattern(RGBData* pDest, size_t destPixelCount,
                                               const RGBData* pPattern, size_t srcPixelCount)
{
    assert ( srcPixelCount > 0 );

    size_t         i = 0;
    const RGBData* pSrc = pPattern;
    while (destPixelCount--)
    {
        *pDest++ = *pSrc++;
        if (++i >= srcPixelCount)
        {
            i = 0;
            pSrc = pPattern;
        }
    }
}

static inline void createInterpolatedPixelPattern(RGBData* pDest, size_t destPixelCount,
                                                  const RGBData* pRgbStart, const RGBData* pRgbStop)
{
    int32_t totalPixels = (int32_t)destPixelCount;
    HSVData hsvStart;
    HSVData hsvStop;

    AnimationBase::rgbToInterpolatableHsv(&hsvStart, pRgbStart);
    AnimationBase::rgbToInterpolatableHsv(&hsvStop, pRgbStop);

    for (int32_t i = 0 ; i < totalPixels ; i++)
    {
        AnimationBase::interpolateHsvToRgb(pDest++, &hsvStart, &hsvStop, i, totalPixels);
    }
}

#endif // ANIMATION_H_
