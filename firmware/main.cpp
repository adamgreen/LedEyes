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


#define LED_COUNT                           16
#define SECONDS_BETWEEN_COUNTER_DUMPS       10

#define ARRAY_SIZE(X) (sizeof(X)/sizeof(X[0]))


static IPixelUpdate*     g_pPixelUpdate;


// Function Prototypes.
static void initAnimation();


int main()
{
    uint32_t lastFlipCount = 0;
    uint32_t lastSetCount = 0;
    static   DigitalOut myled(LED1);
    static   NeoPixel   ledControl(LED_COUNT, p11);
    static   Timer      timer;
    static   Timer      ledTimer;

    initAnimation();
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
        g_pPixelUpdate->updatePixels(ledControl);

        // Flash LED1 on mbed to let user knows that nothing has hung.
        if (ledTimer.read_ms() >= 250)
        {
            myled = !myled;
            ledTimer.reset();
        }
    }
}

static void initAnimation()
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
    g_pPixelUpdate = &flicker;
}
