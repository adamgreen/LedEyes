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
// How many times through the main pumpkin eye animation loop before an eye effect is played? 0 to disable effects.
#define EFFECT_ITERATION                    4
// The number of seconds between dumping of animation performance counters to the serial port.
#define SECONDS_BETWEEN_COUNTER_DUMPS       10
// The number of milliseconds to delay betweenn initial centering of eyes and initial eye wink.
#define MILLISECONDS_FOR_INITIAL_DELAY      2000


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
    STATE_EFFECT_CHOOSER,
    STATE_EFFECT_RUNNING,
    STATE_CRAZY_BLINK_STARTED,
    STATE_DELAY_AFTER_EFFECT,
    STATE_DONE
};

enum EyeEffects
{
    EFFECT_CROSS_EYES,
    EFFECT_ROUND_SPIN,
    EFFECT_CRAZY_SPIN,
    EFFECT_METH_EYES,
    EFFECT_LAZY_EYE,
    EFFECT_CRAZY_BLINK,
    EFFECT_GLOW_EYES,
    EFFECT_MAX
};

static IPixelUpdate*    g_pCandleFlicker;


// Function Prototypes.
static void initCandleFlicker();
static int random(int low, int high);


int main()
{
    uint32_t             lastFlipCount = 0;
    uint32_t             lastSetCount = 0;
    uint32_t             loopCounter = 0;
    EyeEffects           effectCounter = (EyeEffects)0;
    static   NeoPixel    ledControl(LED_COUNT, p11);
    static   Timer       timer;
    static   I2C         i2cEyeMatrices(p9, p10);
    static   EyeMatrices eyes(&i2cEyeMatrices, LEFT_EYE_I2C_ADDRESS, RIGHT_EYE_I2C_ADDRESS);
    EyeState             eyeState = STATE_INIT;
    EyeAnimationBase*    pCurrEyeAnimation = NULL;
    DelayAnimation       delayAnimation(&eyes);
    BlinkAnimation       blinkAnimation(&eyes);
    MoveEyeAnimation     moveEyeAnimation(&eyes);
    CrossEyesAnimation   crossEyesAnimation(&eyes);
    RoundSpinAnimation   roundSpinAnimation(&eyes);
    CrazySpinAnimation   crazySpinAnimation(&eyes);
    MethEyesAnimation    methEyesAnimation(&eyes);
    LazyEyeAnimation     lazyEyeAnimation(&eyes);
    GlowEyesAnimation    glowEyesAnimation(&eyes);

    initCandleFlicker();
    ledControl.start();
    timer.start();
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
            // Start the delay timer before transitioning to the next state.
            i2cEyeMatrices.frequency(400000);
            eyes.init();
            delayAnimation.start(MILLISECONDS_FOR_INITIAL_DELAY);
            pCurrEyeAnimation = &delayAnimation;
            eyeState = STATE_INITIAL_DELAY;
            break;
        case STATE_INITIAL_DELAY:
            // Wait MILLISECONDS_FOR_INITIAL_DELAY msecs (2 seconds) before starting initial wink of the left eye.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_INITIAL_LEFT_EYE_WINK;
                blinkAnimation.start(true, false);
                pCurrEyeAnimation = &blinkAnimation;
            }
            break;
        case STATE_INITIAL_LEFT_EYE_WINK:
            // Winking the left eye.
            // Start winking the right eye once the left wink has completed.
            assert ( pCurrEyeAnimation == &blinkAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_INITIAL_RIGHT_EYE_WINK;
                blinkAnimation.start(false, true);
                pCurrEyeAnimation = &blinkAnimation;
            }
            break;
        case STATE_INITIAL_RIGHT_EYE_WINK:
            // Winking the right eye.
            // Start a 1 second delay once the right wink has completed.
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
            // Transition to STATE_START_LOOP to begin the main eye animation loop.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_START_LOOP;
                pCurrEyeAnimation = NULL;
            }
            break;
        case STATE_START_LOOP:
            // Start the main loop of eye movement animations.
            // Increment the loop counter and start moving both eyes to a random position.
            loopCounter++;
            eyeState = STATE_MOVING_EYES;
            moveEyeAnimation.start(random(-2, 2),
                                   random(-2, 2),
                                   50);
            pCurrEyeAnimation = &moveEyeAnimation;
            break;
        case STATE_MOVING_EYES:
            // Moving eyes around to random offset.
            // Wait for the eye movement to complete and then start a random delay between 2.5 and 3 seconds.
            assert ( pCurrEyeAnimation == &moveEyeAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                delayAnimation.start(random(5, 6) * 500);
                pCurrEyeAnimation = &delayAnimation;
                eyeState = STATE_DELAY_AFTER_MOVE;
            }
            break;
        case STATE_DELAY_AFTER_MOVE:
            // Random delay after eye movement.
            // Wait for delay to complete and then randomly decide to either:
            //  Blink both eyes
            //      - or -
            //  Skip blink and enter STATE_EFFECT_CHOOSER to determine if a special eye animation should be started.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                if (random(0, 4) == 0)
                {
                    blinkAnimation.start(true, true);
                    pCurrEyeAnimation = &blinkAnimation;
                    eyeState = STATE_BLINKING;
                }
                else
                {
                    eyeState = STATE_EFFECT_CHOOSER;
                    pCurrEyeAnimation = NULL;
                }
            }
            break;
        case STATE_BLINKING:
            // Waiting for randomly eye blink to complete.
            // Wait for eye blink to complete and then start a half second delay.
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
            // Enter STATE_EFFECT_CHOOSER state to determine if a special eye animation should be started.
            assert ( pCurrEyeAnimation == &delayAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_EFFECT_CHOOSER;
                pCurrEyeAnimation = NULL;
            }
            break;
        case STATE_EFFECT_CHOOSER:
            // Check to see if it is time to start a special eye effect and if so, get that animation started.
            assert ( pCurrEyeAnimation == NULL );
            if (EFFECT_ITERATION == 0 || loopCounter < EFFECT_ITERATION)
            {
                // Not running an effect this iteration so go back to start of main animation loop.
                eyeState = STATE_START_LOOP;
                pCurrEyeAnimation = NULL;
            }
            else
            {
                // Start the appropriate eye animation.
                switch (effectCounter)
                {
                case EFFECT_CROSS_EYES:
                    crossEyesAnimation.start();
                    pCurrEyeAnimation = &crossEyesAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                case EFFECT_ROUND_SPIN:
                    roundSpinAnimation.start();
                    pCurrEyeAnimation = &roundSpinAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                case EFFECT_CRAZY_SPIN:
                    crazySpinAnimation.start();
                    pCurrEyeAnimation = &crazySpinAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                case EFFECT_METH_EYES:
                    methEyesAnimation.start();
                    pCurrEyeAnimation = &methEyesAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                case EFFECT_LAZY_EYE:
                    lazyEyeAnimation.start();
                    pCurrEyeAnimation = &lazyEyeAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                case EFFECT_CRAZY_BLINK:
                    blinkAnimation.start(true, false);
                    pCurrEyeAnimation = &blinkAnimation;
                    // Enters a different state than other effects since it needs to start winking the right eye
                    // once the left eye completes its wink.
                    eyeState = STATE_CRAZY_BLINK_STARTED;
                    break;
                case EFFECT_GLOW_EYES:
                    glowEyesAnimation.start(3);
                    pCurrEyeAnimation = &glowEyesAnimation;
                    eyeState = STATE_EFFECT_RUNNING;
                    break;
                default:
                    assert ( effectCounter < EFFECT_MAX );
                    eyeState = STATE_START_LOOP;
                    break;
                }

                loopCounter = 0;
                effectCounter = (EyeEffects)(effectCounter + 1);
                if (effectCounter >= EFFECT_MAX)
                    effectCounter = (EyeEffects)0;
            }
            break;
        case STATE_CRAZY_BLINK_STARTED:
            // The crazy blink has started with the left pupil.
            // Wait for left wink to complete and then start the right wink.
            assert ( pCurrEyeAnimation == &blinkAnimation );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_EFFECT_RUNNING;
                blinkAnimation.start(false, true);
                pCurrEyeAnimation = &blinkAnimation;
            }
            break;
        case STATE_EFFECT_RUNNING:
            // An eye effect animation is now running.
            // Wait for it to complete and then start a 1 second delay.
            assert ( pCurrEyeAnimation != NULL );
            if (pCurrEyeAnimation->isDone())
            {
                eyeState = STATE_DELAY_AFTER_EFFECT;
                delayAnimation.start(1000);
                pCurrEyeAnimation = &delayAnimation;
            }
            break;
        case STATE_DELAY_AFTER_EFFECT:
            // Delay for 1 second after an effect and then loop around to the top of the main animation loop again.
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
    }
}

static void initCandleFlicker()
{
    static FlickerAnimation<LED_COUNT> flicker;
    static FlickerProperties           flickerProperties;

    flickerProperties.timeMin = 5;
    flickerProperties.timeMax = 250;
    flickerProperties.stayBrightFactor = 5;
    flickerProperties.brightnessMin = 128;
    flickerProperties.brightnessMax = 200;
    flickerProperties.baseRGBColour = DARK_ORANGE;
    flicker.setProperties(&flickerProperties);
    g_pCandleFlicker = &flicker;
}

// Returns a random number between low and high, inclusively.
static int random(int low, int high)
{
    assert ( high > low );
    int delta = high - low + 1;
    return rand() % delta + low;
}
