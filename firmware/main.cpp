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
#include <mbed.h>
#include "Animation.h"
#include "NeoPixel.h"


#define LED_COUNT                           5
#define SECONDS_BETWEEN_ANIMATION_SWITCH    2
#define DUMP_COUNTERS                       1

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))

// The animations that I currently have defined for this sample.
enum Animations
{
    Solid_White,
    Solid_Red,
    Solid_Green,
    Solid_Blue_White,
    Solid_Red_Green,
    Solid_Red_Green_White,
    Solid_Red_Orange_Yellow_Green_Blue,
    Chase_Blue_White,
    Chase_Red_Green,
    Chase_Red_Green_White,
    Chase_Red_Orange_Yellow_Green_Blue,
    Throbbing_Red,
    Throbbing_Green,
    Throbbing_White,
    Fade_Blue_White,
    Fade_Red_Green,
    Fade_Red_Green_White,
    Fade_Red_Orange_Yellow_Green_Blue,
    Fade_Rainbow,
    Twinkle_White,
    Twinkle_Red,
    Twinkle_Green,
    Twinkle_AnyColor,
    Candle_Flicker,
    Max_Animation
};

static Animations        g_currAnimation = Solid_White;
static IPixelUpdate*     g_pPixelUpdate;


// Function Prototypes.
static void updateAnimation();
static void advanceToNextAnimation();


int main()
{
    uint32_t lastFlipCount = 0;
    uint32_t lastSetCount = 0;
    static   DigitalOut myled(LED1);
    static   NeoPixel   ledControl(LED_COUNT, p11);
    static   Timer      timer;
    static   Timer      ledTimer;

    updateAnimation();
    ledControl.start();

    timer.start();
    ledTimer.start();

    while(1)
    {

        if (timer.read_ms() > SECONDS_BETWEEN_ANIMATION_SWITCH * 1000)
        {
            uint32_t currSetCount = ledControl.getSetCount();
            uint32_t setCount =  currSetCount - lastSetCount;
            uint32_t currFlipCount = ledControl.getFlipCount();
            uint32_t flipCount = currFlipCount - lastFlipCount;

            if (DUMP_COUNTERS)
            {
                printf("flips: %lu/sec    sets: %lu/sec\n",
                       flipCount / SECONDS_BETWEEN_ANIMATION_SWITCH,
                       setCount / SECONDS_BETWEEN_ANIMATION_SWITCH);
            }

            timer.reset();
            advanceToNextAnimation();

            lastSetCount = currSetCount;
            lastFlipCount = currFlipCount;
        }
        g_pPixelUpdate->updatePixels(ledControl);

        if (ledTimer.read_ms() >= 250)
        {
            myled = !myled;
            ledTimer.reset();
        }
    }
}

static void updateAnimation()
{
    static Animation<LED_COUNT>        animation;
    static TwinkleAnimation<LED_COUNT> twinkle;
    static TwinkleProperties           twinkleProperties;
    static FlickerAnimation<LED_COUNT> flicker;
    static FlickerProperties           flickerProperties;
    static RGBData                     pixels1[LED_COUNT];
    static RGBData                     pixels2[LED_COUNT];
    static RGBData                     pixels3[LED_COUNT];
    static RGBData                     pixels4[LED_COUNT];
    static RGBData                     pixels5[LED_COUNT];
    static AnimationKeyFrame           keyFrames[5];

    switch (g_currAnimation)
    {
    case Solid_White:
        {
            RGBData pattern[] = { WHITE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Red:
        {
            RGBData pattern[] = { RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Green:
        {
            RGBData pattern[] = { GREEN };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Blue_White:
        {
            RGBData pattern[] = { BLUE, WHITE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Red_Green:
        {
            RGBData pattern[] = { RED, GREEN };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Red_Green_White:
        {
            RGBData pattern[] = { RED, GREEN, WHITE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Solid_Red_Orange_Yellow_Green_Blue:
        {
            RGBData pattern[] = { RED, DARK_ORANGE, YELLOW, GREEN, BLUE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern, ARRAY_SIZE(pattern));
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    case Chase_Blue_White:
        {
            RGBData pattern1[] = { BLUE, WHITE };
            RGBData pattern2[] = { WHITE, BLUE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 250, false};
            keyFrames[1] = {pixels2, 250, false};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Chase_Red_Green:
        {
            RGBData pattern1[] = { RED, GREEN };
            RGBData pattern2[] = { GREEN, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 250, false};
            keyFrames[1] = {pixels2, 250, false};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Chase_Red_Green_White:
        {
            RGBData pattern1[] = { RED, GREEN, WHITE };
            RGBData pattern2[] = { WHITE, RED, GREEN };
            RGBData pattern3[] = { GREEN, WHITE, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            createRepeatingPixelPattern(pixels3, ARRAY_SIZE(pixels3), pattern3, ARRAY_SIZE(pattern3));
            keyFrames[0] = {pixels1, 250, false};
            keyFrames[1] = {pixels2, 250, false};
            keyFrames[2] = {pixels3, 250, false};
            animation.setKeyFrames(keyFrames, 3);
            g_pPixelUpdate = &animation;
            break;
        }
    case Chase_Red_Orange_Yellow_Green_Blue:
        {
            RGBData pattern1[] = { RED, DARK_ORANGE, YELLOW, GREEN, BLUE };
            RGBData pattern2[] = { BLUE, RED, DARK_ORANGE, YELLOW, GREEN };
            RGBData pattern3[] = { GREEN, BLUE, RED, DARK_ORANGE, YELLOW };
            RGBData pattern4[] = { YELLOW, GREEN, BLUE, RED, DARK_ORANGE };
            RGBData pattern5[] = { DARK_ORANGE, YELLOW, GREEN, BLUE, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            createRepeatingPixelPattern(pixels3, ARRAY_SIZE(pixels3), pattern3, ARRAY_SIZE(pattern3));
            createRepeatingPixelPattern(pixels4, ARRAY_SIZE(pixels4), pattern4, ARRAY_SIZE(pattern4));
            createRepeatingPixelPattern(pixels5, ARRAY_SIZE(pixels5), pattern5, ARRAY_SIZE(pattern5));
            keyFrames[0] = {pixels1, 250, false};
            keyFrames[1] = {pixels2, 250, false};
            keyFrames[2] = {pixels3, 250, false};
            keyFrames[3] = {pixels4, 250, false};
            keyFrames[4] = {pixels5, 250, false};
            animation.setKeyFrames(keyFrames, 5);
            g_pPixelUpdate = &animation;
            break;
        }
    case Throbbing_Red:
        {
            RGBData pattern1[] = { RGBData(8, 0, 0) };
            RGBData pattern2[] = { RGBData(255, 0, 0) };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 1000, true};
            keyFrames[1] = {pixels2, 1000, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Throbbing_Green:
        {
            RGBData pattern1[] = { RGBData(0, 8, 0) };
            RGBData pattern2[] = { RGBData(0, 255, 0) };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 1000, true};
            keyFrames[1] = {pixels2, 1000, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Throbbing_White:
        {
            RGBData pattern1[] = { RGBData(8, 8, 8) };
            RGBData pattern2[] = { RGBData(255, 255, 255) };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 1000, true};
            keyFrames[1] = {pixels2, 1000, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Fade_Blue_White:
        {
            RGBData pattern1[] = { BLUE, WHITE };
            RGBData pattern2[] = { WHITE, BLUE };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 250, true};
            keyFrames[1] = {pixels2, 250, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Fade_Red_Green:
        {
            RGBData pattern1[] = { RED, GREEN };
            RGBData pattern2[] = { GREEN, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            keyFrames[0] = {pixels1, 250, true};
            keyFrames[1] = {pixels2, 250, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Fade_Red_Green_White:
        {
            RGBData pattern1[] = { RED, GREEN, WHITE };
            RGBData pattern2[] = { WHITE, RED, GREEN };
            RGBData pattern3[] = { GREEN, WHITE, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            createRepeatingPixelPattern(pixels3, ARRAY_SIZE(pixels3), pattern3, ARRAY_SIZE(pattern3));
            keyFrames[0] = {pixels1, 250, true};
            keyFrames[1] = {pixels2, 250, true};
            keyFrames[2] = {pixels3, 250, true};
            animation.setKeyFrames(keyFrames, 3);
            g_pPixelUpdate = &animation;
            break;
        }
    case Fade_Red_Orange_Yellow_Green_Blue:
        {
            RGBData pattern1[] = { RED, DARK_ORANGE, YELLOW, GREEN, BLUE };
            RGBData pattern2[] = { BLUE, RED, DARK_ORANGE, YELLOW, GREEN };
            RGBData pattern3[] = { GREEN, BLUE, RED, DARK_ORANGE, YELLOW };
            RGBData pattern4[] = { YELLOW, GREEN, BLUE, RED, DARK_ORANGE };
            RGBData pattern5[] = { DARK_ORANGE, YELLOW, GREEN, BLUE, RED };
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, ARRAY_SIZE(pattern1));
            createRepeatingPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, ARRAY_SIZE(pattern2));
            createRepeatingPixelPattern(pixels3, ARRAY_SIZE(pixels3), pattern3, ARRAY_SIZE(pattern3));
            createRepeatingPixelPattern(pixels4, ARRAY_SIZE(pixels4), pattern4, ARRAY_SIZE(pattern4));
            createRepeatingPixelPattern(pixels5, ARRAY_SIZE(pixels5), pattern5, ARRAY_SIZE(pattern5));
            keyFrames[0] = {pixels1, 250, true};
            keyFrames[1] = {pixels2, 250, true};
            keyFrames[2] = {pixels3, 250, true};
            keyFrames[3] = {pixels4, 250, true};
            keyFrames[4] = {pixels5, 250, true};
            animation.setKeyFrames(keyFrames, 5);
            g_pPixelUpdate = &animation;
            break;
        }
    case Fade_Rainbow:
        {
            RGBData pattern1[] = { RED };
            RGBData pattern2[] = { VIOLET };
            createInterpolatedPixelPattern(pixels1, ARRAY_SIZE(pixels1), pattern1, pattern2);
            createInterpolatedPixelPattern(pixels2, ARRAY_SIZE(pixels2), pattern2, pattern1);
            keyFrames[0] = {pixels1, 1000, true};
            keyFrames[1] = {pixels2, 1000, true};
            animation.setKeyFrames(keyFrames, 2);
            g_pPixelUpdate = &animation;
            break;
        }
    case Twinkle_White:
        {
            twinkleProperties.lifetimeMin = 128;
            twinkleProperties.lifetimeMax = 250;
            twinkleProperties.probability = 250;
            twinkleProperties.hueMin = 0;
            twinkleProperties.hueMax = 0;
            twinkleProperties.saturationMin = 0;
            twinkleProperties.saturationMax = 0;
            twinkleProperties.valueMin = 128;
            twinkleProperties.valueMax = 255;
            twinkle.setProperties(&twinkleProperties);
            g_pPixelUpdate = &twinkle;
            break;
        }
    case Twinkle_Red:
        {
            twinkleProperties.lifetimeMin = 128;
            twinkleProperties.lifetimeMax = 250;
            twinkleProperties.probability = 250;
            twinkleProperties.hueMin = 0;
            twinkleProperties.hueMax = 0;
            twinkleProperties.saturationMin = 255;
            twinkleProperties.saturationMax = 255;
            twinkleProperties.valueMin = 128;
            twinkleProperties.valueMax = 255;
            twinkle.setProperties(&twinkleProperties);
            g_pPixelUpdate = &twinkle;
            break;
        }
    case Twinkle_Green:
        {
            twinkleProperties.lifetimeMin = 128;
            twinkleProperties.lifetimeMax = 250;
            twinkleProperties.probability = 250;
            twinkleProperties.hueMin = 84;
            twinkleProperties.hueMax = 84;
            twinkleProperties.saturationMin = 255;
            twinkleProperties.saturationMax = 255;
            twinkleProperties.valueMin = 128;
            twinkleProperties.valueMax = 255;
            twinkle.setProperties(&twinkleProperties);
            g_pPixelUpdate = &twinkle;
            break;
        }
    case Twinkle_AnyColor:
        {
            twinkleProperties.lifetimeMin = 128;
            twinkleProperties.lifetimeMax = 250;
            twinkleProperties.probability = 250;
            twinkleProperties.hueMin = 0;
            twinkleProperties.hueMax = 255;
            twinkleProperties.saturationMin = 0;
            twinkleProperties.saturationMax = 255;
            twinkleProperties.valueMin = 128;
            twinkleProperties.valueMax = 255;
            twinkle.setProperties(&twinkleProperties);
            g_pPixelUpdate = &twinkle;
            break;
        }
    case Candle_Flicker:
        {
            flickerProperties.timeMin = 2;
            flickerProperties.timeMax = 3;
            flickerProperties.stayBrightFactor = 2;
            flickerProperties.brightnessMin = 128;
            flickerProperties.brightnessMax = 255;
            flickerProperties.baseRGBColour = DARK_ORANGE;
            flicker.setProperties(&flickerProperties);
            g_pPixelUpdate = &flicker;
            break;
        }
    default:
        {
            RGBData pattern = BLACK;
            createRepeatingPixelPattern(pixels1, ARRAY_SIZE(pixels1), &pattern, 1);
            keyFrames[0] = {pixels1, 0x7FFFFFFF, false};
            animation.setKeyFrames(keyFrames, 1);
            g_pPixelUpdate = &animation;
            break;
        }
    }
}

static void advanceToNextAnimation()
{
    g_currAnimation = (Animations)(g_currAnimation + 1);
    if (g_currAnimation >= Max_Animation)
    {
        g_currAnimation = Solid_White;
    }
    updateAnimation();
}
