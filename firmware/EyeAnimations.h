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



struct PupilPosition
{
    int x;
    int y;
};

struct PupilKeyFrame
{
    PupilPosition left;
    PupilPosition right;
    uint32_t      frameDelayStart;
    int32_t       frameDelayStep;
    bool          interpolate;
};

class EyeMatrices
{
public:
    EyeMatrices(I2C* pI2C, uint8_t leftEyeAddress = 0x70, uint8_t rightEyeAddress = 0x71);

    void init()
    {
        PupilPosition pos = {0, 0};
        displayEyes(&pos, &pos);
    }

    // UNDONE: Might be able to make these protected or just remove now that I have drawRow.
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
        MIN = -5,
        MAX = 5
    };

    enum PupilEnum
    {
        LEFT = 0,
        RIGHT = 1,
        PUPIL_COUNT = 2
    };

    void displayEyes(const PupilPosition* pLeftPos, const PupilPosition* pRightPos);

    void drawRow(PupilEnum pupil, int row, uint8_t rowData)
    {
        switch (pupil)
        {
        case LEFT:
            m_leftEye.drawRow(row, rowData);
            break;
        case RIGHT:
            m_rightEye.drawRow(row, rowData);
            break;
        default:
            assert ( pupil == LEFT || pupil == RIGHT );
            break;
        }
    }

    static PupilPosition getValidPupilPosition(const PupilPosition* pPos)
    {
        int x = pPos->x;
        int y = pPos->y;
        PupilPosition validPos = {x, y};

        if (x > MAX)
            validPos.x = MAX;
        else if (x < MIN)
            validPos.x = MIN;

        if (y > MAX)
            validPos.y = MAX;
        else if (y < MIN)
            validPos.y = MIN;
        return validPos;
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

    // UNDONE: Can I make m_eyeCurrent protected?
    uint8_t            m_eyeCurrent[PUPIL_COUNT][8];
    PupilPosition      m_currentPos[PUPIL_COUNT];
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



class PupilAnimation : public EyeAnimationBase
{
public:
    PupilAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }
    ~PupilAnimation()
    {
    }

    void start(const PupilKeyFrame* pKeyFrames, size_t keyFrameCount);
    virtual void run();

    virtual bool isDone()
    {
        return m_isDone;
    }

protected:
    void startNextFrame();

    const PupilKeyFrame* m_pFirstKeyFrame;
    const PupilKeyFrame* m_pLastKeyFrame;
    const PupilKeyFrame* m_pCurrKeyFrame;
    PupilPosition        m_startPos[EyeMatrices::PUPIL_COUNT];
    int                  m_index;
    int                  m_steps;
    uint32_t             m_frameDelay;
    int32_t              m_frameDelayStep;
    float                m_changeX[EyeMatrices::PUPIL_COUNT];
    float                m_changeY[EyeMatrices::PUPIL_COUNT];
    bool                 m_isDone;
};

class MoveEyeAnimation : public PupilAnimation
{
public:
    MoveEyeAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }
    ~MoveEyeAnimation()
    {
    }

    void start(int newX, int newY, uint32_t stepDelay);

protected:
    PupilKeyFrame m_keyFrames[1];
};
