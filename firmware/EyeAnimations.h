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
/* Eye animations using Adafruit's 8x8 LED Matrix.
   Ported from Michal T Janyst's Led Eyes project (https://github.com/michaltj/LedEyes)
*/
#include <assert.h>
#include <mbed.h>
#include "Adafruit_LEDBackpack.h"


class EyeMatrices
{
public:
    EyeMatrices(I2C* pI2C, uint8_t leftEyeAddress = 0x70, uint8_t rightEyeAddress = 0x71);

    void init()
    {
        displayEyes(0, 0);
    }

    Adafruit_8x8matrix* left()
    {
        return &m_leftEye;
    }

    Adafruit_8x8matrix* right()
    {
        return &m_rightEye;
    }

    enum PupilLimits
    {
        MIN = -2,
        MAX = 2
    };

    /*
      This method displays eyeball with pupil offset by X, Y values from center position.
      Valid X and Y range is [MIN,MAX]
      Both LED modules will show identical eyes
    */
    void displayEyes(int offsetX, int offsetY);

    /*
      This method corrects provided coordinate value
    */
    static int getValidValue(int value)
    {
      if (value > MAX)
        return MAX;
      else if (value < MIN)
        return MIN;
      else
        return value;
    }

    void writeDisplays()
    {
        m_leftEye.writeDisplay();
        m_rightEye.writeDisplay();
    }

    uint32_t getCurrentTime()
    {
        return m_timer.read_ms();
    }

    uint8_t            m_eyeCurrent[8];
    int                m_currentX;
    int                m_currentY;
protected:
    Adafruit_8x8matrix m_leftEye;
    Adafruit_8x8matrix m_rightEye;
    Timer              m_timer;
};


class EyeAnimationBase
{
public:
    EyeAnimationBase(EyeMatrices* pEyes)
    {
        m_pEyes = pEyes;
        m_startTime = 0;
        m_desiredDelay = 0;
    }
    virtual ~EyeAnimationBase()
    {
    }

    void startDelay(uint32_t desiredDelay)
    {
        m_startTime = m_pEyes->getCurrentTime();
        m_desiredDelay = desiredDelay;
    }

    bool isDelayDone()
    {
        uint32_t delta = m_pEyes->getCurrentTime() - m_startTime;
        return delta >= m_desiredDelay;
    }

    virtual void run() = 0;
    virtual bool isDone() = 0;

protected:
    EyeMatrices* m_pEyes;
    uint32_t     m_startTime;
    uint32_t     m_desiredDelay;
};


class DelayAnimation : public EyeAnimationBase
{
public:
    DelayAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
    }
    ~DelayAnimation()
    {
    }

    void start(uint32_t desiredDelay)
    {
        startDelay(desiredDelay);
    }

    virtual void run()
    {
    }

    virtual bool isDone()
    {
        return isDelayDone();
    }

protected:
};


class BlinkAnimation : public EyeAnimationBase
{
public:
    BlinkAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }
    ~BlinkAnimation()
    {
    }

    void start(bool leftBlink = true, bool rightBlink = true);
    virtual void run();

    virtual bool isDone()
    {
        return m_isDone;
    }

protected:
    enum State
    {
        EYE_CLOSING,
        EYE_OPENING,
        DONE
    };

    int      m_index;
    State    m_state;
    bool     m_leftBlink;
    bool     m_rightBlink;
    bool     m_isDone;
};


class MoveEyeAnimation : public EyeAnimationBase
{
public:
    MoveEyeAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }
    ~MoveEyeAnimation()
    {
    }

    void start(int newX, int newY, uint32_t stepDelay);
    virtual void run();

    virtual bool isDone()
    {
        return m_isDone;
    }

protected:
    uint32_t    m_stepDelay;
    int         m_startX;
    int         m_startY;
    int         m_dirX;
    int         m_dirY;
    int         m_steps;
    float       m_changeX;
    float       m_changeY;
    int         m_index;
    bool        m_isDone;
};
