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
#include "Adafruit_LEDBackpack.h"
#include "Animation.h"
#include "EyeAnimations.h"
#include "NeoPixel.h"


// The number of LEDs in the Adafruit ring used for the candle.
#define LED_COUNT                           16
// The 7-bit I2C address for the left eye 8x8 matrix.
#define LEFT_EYE_I2C_ADDRESS                0x70
// The 7-bit I2C address for the right eye 8x8 matrix.
#define RIGHT_EYE_I2C_ADDRESS               0x71
// The number of seconds between dumping of animation performance counters to the serial port.
#define SECONDS_BETWEEN_COUNTER_DUMPS       10
// The number of milliseconds to delay betweenn initial centering of eyes and initial eye wink.
#define MILLISECONDS_FOR_INITIAL_DELAY      2000

// Utility macros.
#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))


enum EyeState
{
    STATE_INIT,
    STATE_INITIAL_DELAY,
    STATE_INITIAL_LEFT_EYE_WINK,
    STATE_INITIAL_RIGHT_EYE_WINK,
    STATE_DELAY_AFTER_WINK,
    STATE_START_LOOP,
    STATE_MOVING_EYES,
    STATE_DELAY_AFTER_MOVE,
    STATE_BLINKING,
    STATE_DELAY_AFTER_BLINK,
    STATE_DONE // UNDONE:
};

static IPixelUpdate*    g_pCandleFlicker;


// Function Prototypes.
static void initCandleFlicker();
static int random(int low, int high);


int main()
{
    uint32_t             lastFlipCount = 0;
    uint32_t             lastSetCount = 0;
    static   DigitalOut  myled(LED1);
    static   NeoPixel    ledControl(LED_COUNT, p11);
    static   Timer       timer;
    static   Timer       ledTimer;
    static   I2C         i2cEyeMatrices(p9, p10);
    static   EyeMatrices eyes(&i2cEyeMatrices, LEFT_EYE_I2C_ADDRESS, RIGHT_EYE_I2C_ADDRESS);
    EyeState             eyeState = STATE_INIT;
    EyeAnimationBase*    pCurrEyeAnimation = NULL;
    DelayAnimation       delayAnimation(&eyes);
    BlinkAnimation       blinkAnimation(&eyes);
    MoveEyeAnimation     moveEyeAnimation(&eyes);

    initCandleFlicker();
    ledControl.start();

    timer.start();
    ledTimer.start();

    while(1)
    {
        if (SECONDS_BETWEEN_COUNTER_DUMPS > 0 && timer.read_ms() > SECONDS_BETWEEN_COUNTER_DUMPS * 1000)
        {
            uint32_t currSetCount = ledControl.getSetCount();
            uint32_t setCount =  currSetCount - lastSetCount;
            uint32_t currFlipCount = ledControl.getFlipCount();
            uint32_t flipCount = currFlipCount - lastFlipCount;

            printf("flips: %lu/sec    sets: %lu/sec\n",
                   flipCount / SECONDS_BETWEEN_COUNTER_DUMPS,
                   setCount / SECONDS_BETWEEN_COUNTER_DUMPS);

            timer.reset();

            lastSetCount = currSetCount;
            lastFlipCount = currFlipCount;
        }
        g_pCandleFlicker->updatePixels(ledControl);

        // Run the eye animation state machine.
        if (pCurrEyeAnimation)
            pCurrEyeAnimation->run();
        switch (eyeState)
        {
        case STATE_INIT:
            // Center the eyes to initialize the state.
            eyes.init();
            delayAnimation.start(MILLISECONDS_FOR_INITIAL_DELAY);
            pCurrEyeAnimation = &delayAnimation;
            eyeState = STATE_INITIAL_DELAY;
            break;
        case STATE_INITIAL_DELAY:
            // Wait MILLISECONDS_FOR_INITIAL_DELAY msecs (2 seconds) before initial wink.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_INITIAL_LEFT_EYE_WINK;
                blinkAnimation.start(true, false);
                pCurrEyeAnimation = &blinkAnimation;
            }
            break;
        case STATE_INITIAL_LEFT_EYE_WINK:
            // Wink the left eye.
            assert ( pCurrEyeAnimation == &blinkAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_INITIAL_RIGHT_EYE_WINK;
                blinkAnimation.start(false, true);
                pCurrEyeAnimation = &blinkAnimation;
            }
            break;
        case STATE_INITIAL_RIGHT_EYE_WINK:
            // Then wink the right eye.
            assert ( pCurrEyeAnimation == &blinkAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_DELAY_AFTER_WINK;
                delayAnimation.start(1000);
                pCurrEyeAnimation = &delayAnimation;
            }
            break;
        case STATE_DELAY_AFTER_WINK:
            // Delay for 1 second after initial wink.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_START_LOOP;
                pCurrEyeAnimation = NULL;
            }
            break;
        case STATE_START_LOOP:
            // Start the main loop of eye movement animations.
            eyeState = STATE_MOVING_EYES;
            moveEyeAnimation.start(random(EyeMatrices::PupilLimits::MIN, EyeMatrices::PupilLimits::MAX + 1),
                                   random(EyeMatrices::PupilLimits::MIN, EyeMatrices::PupilLimits::MAX + 1),
                                   50);
            pCurrEyeAnimation = &moveEyeAnimation;
            break;
        case STATE_MOVING_EYES:
            // Moving eyes around to random offset.
            assert ( pCurrEyeAnimation == &moveEyeAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                delayAnimation.start(random(5, 7) * 500);
                pCurrEyeAnimation = &delayAnimation;
                eyeState = STATE_DELAY_AFTER_MOVE;
            }
            break;
        case STATE_DELAY_AFTER_MOVE:
            // Random delay after eye movement.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                if (random(0, 5) == 0)
                {
                    blinkAnimation.start(true, true);
                    pCurrEyeAnimation = &blinkAnimation;
                    eyeState = STATE_BLINKING;
                }
                else
                {
                    eyeState = STATE_START_LOOP;
                    pCurrEyeAnimation = NULL;
                }
            }
            break;
        case STATE_BLINKING:
            // Sometimes we will enter this state from STATE_DELAY_AFTER_MOVE to blink.
            assert ( pCurrEyeAnimation == &blinkAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_DELAY_AFTER_BLINK;
                delayAnimation.start(500);
                pCurrEyeAnimation = &delayAnimation;
            }
            break;
        case STATE_DELAY_AFTER_BLINK:
            // Delay for 0.5 second after blink.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_START_LOOP;
                pCurrEyeAnimation = NULL;
            }
            break;
        case STATE_DONE:
            break;
        }

        // Flash LED1 on mbed to let user knows that nothing has hung.
        if (ledTimer.read_ms() >= 250)
        {
            myled = !myled;
            ledTimer.reset();
        }
    }
}

static void initCandleFlicker()
{
    static FlickerAnimation<LED_COUNT> flicker;
    static FlickerProperties           flickerProperties;

    flickerProperties.timeMin = 5;
    flickerProperties.timeMax = 250;
    flickerProperties.stayBrightFactor = 5;
    flickerProperties.brightnessMin = 140;
    flickerProperties.brightnessMax = 255;
    flickerProperties.baseRGBColour = DARK_ORANGE;
    flicker.setProperties(&flickerProperties);
    g_pCandleFlicker = &flicker;
}

static int random(int low, int high)
{
    assert ( high > low );
    int delta = high - low + 1;
    return rand() % delta + low;
}
