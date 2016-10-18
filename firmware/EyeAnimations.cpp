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
#include "EyeAnimations.h"
#include "util.h"

// The number of milliseconds to delay between eye blink frames.
#define MILLISECONDS_FOR_BLINK_DELAY        40

// define eye ball without pupil
static const uint8_t g_eyeBall[8] =
{
    0x3C, //B00111100,
    0x7E, //B01111110,
    0xFF, //B11111111,
    0xFF, //B11111111,
    0xFF, //B11111111,
    0xFF, //B11111111,
    0x7E, //B01111110,
    0x3C  //B00111100
};

// The mask for turning off pixels to represent the pupil.
static const uint8_t g_eyePupil = 0xE7; //B11100111;



EyeMatrices::EyeMatrices(I2C* pI2C, uint8_t leftEyeAddress /* = 0x70 */, uint8_t rightEyeAddress /* = 0x71 */) :
    m_leftEye(pI2C),
    m_rightEye(pI2C)
{
    // Initialize the LED eye matrices.
    m_leftEye.begin(leftEyeAddress);
    m_leftEye.setBrightness(0);
    m_rightEye.begin(rightEyeAddress);
    m_rightEye.setBrightness(0);
    m_timer.start();
}

void EyeMatrices::displayEyes(const PupilPosition* pLeftPos, const PupilPosition* pRightPos)
{
    PupilPosition pos[PUPIL_COUNT];
    pos[LEFT] = getValidPupilPosition(pLeftPos);
    pos[RIGHT] = getValidPupilPosition(pRightPos);

    // calculate indices for pupil rows (perform offset Y)
    int     row1[PUPIL_COUNT];
    int     row2[PUPIL_COUNT];
    uint8_t pupilRow[PUPIL_COUNT] = {g_eyePupil, g_eyePupil};
    uint8_t pupilRow1[PUPIL_COUNT] = {0, 0};
    uint8_t pupilRow2[PUPIL_COUNT] = {0, 0};

    for (PupilEnum pupil = LEFT ; pupil <= RIGHT ; pupil = (PupilEnum)(pupil + 1))
    {
        row1[pupil] = 3 - pos[pupil].y;
        row2[pupil] = 4 - pos[pupil].y;

        // define pupil row
        int x = pos[pupil].x;
        if (x > 0)
            pupilRow[pupil] = (g_eyePupil << x) | ((1 << x) - 1);
        else if (x < 0)
            pupilRow[pupil] = ((int8_t)g_eyePupil >> -x);

        // pupil row cannot have 1s where eyeBall has 0s
        if (row1[pupil] >= 0 && row1[pupil] <= 7)
            pupilRow1[pupil] = pupilRow[pupil] & g_eyeBall[row1[pupil]];
        if (row2[pupil] >= 0 && row2[pupil] <= 7)
            pupilRow2[pupil] = pupilRow[pupil] & g_eyeBall[row2[pupil]];

        // display on LCD matrix, update to eyeCurrent
        for (int r = 0; r < 8 ; r++)
        {
            if (r == row1[pupil])
            {
                drawRow(pupil, r, pupilRow1[pupil]);
                m_eyeCurrent[pupil][r] = pupilRow1[pupil];
            }
            else if (r == row2[pupil])
            {
                drawRow(pupil, r, pupilRow2[pupil]);
                m_eyeCurrent[pupil][r] = pupilRow2[pupil];
            }
            else
            {
                drawRow(pupil, r, g_eyeBall[r]);
                m_eyeCurrent[pupil][r] = g_eyeBall[r];
            }
        }
    }

    // update current X and Y
    m_currentPos[LEFT] = pos[LEFT];
    m_currentPos[RIGHT] = pos[RIGHT];

    writeDisplays();
}

void EyeMatrices::turnRowOffTemporarily(PupilEnum pupil, int row)
{
    Adafruit_8x8matrix* pEye = eye(pupil);
    assert ( row >= 0 && row < 8 );
    pEye->drawRow(row, 0);
}

void EyeMatrices::restoreRow(PupilEnum pupil, int row)
{
    Adafruit_8x8matrix* pEye = eye(pupil);
    assert ( row >= 0 && row < 8 );
    pEye->drawRow(row, m_eyeCurrent[pupil][row]);
}



void BlinkAnimation::start(bool leftBlink /* = true */, bool rightBlink /* = true */)
{
    m_leftBlink = leftBlink;
    m_rightBlink = rightBlink;

    // Nothing to do if not being asked to blink either eye.
    if (!m_leftBlink && !m_rightBlink)
    {
        m_isDone = true;
        return;
    }

    // Start the eye(s) closing on the next call to run().
    m_isDone = false;
    m_state = EYE_CLOSING;
    m_index = 0;
    startDelay(0);
}

void BlinkAnimation::run()
{
    // Just return if the animation is still waiting for a delay between frames.
    if (!isDelayDone())
        return;

    switch (m_state)
    {
    case EYE_CLOSING:
        if (m_leftBlink)
        {
            m_pEyes->turnRowOffTemporarily(EyeMatrices::LEFT, m_index);
            m_pEyes->turnRowOffTemporarily(EyeMatrices::LEFT, 7-m_index);
        }
        if (m_rightBlink)
        {
            m_pEyes->turnRowOffTemporarily(EyeMatrices::RIGHT, m_index);
            m_pEyes->turnRowOffTemporarily(EyeMatrices::RIGHT, 7-m_index);
        }
        m_pEyes->writeDisplays();

        startDelay(MILLISECONDS_FOR_BLINK_DELAY);

        m_index++;
        if (m_index > 3)
        {
            m_state = EYE_OPENING;
            m_index = 3;
        }
        break;
    case EYE_OPENING:
        if (m_leftBlink)
        {
            m_pEyes->restoreRow(EyeMatrices::LEFT, m_index);
            m_pEyes->restoreRow(EyeMatrices::LEFT, 7-m_index);
        }
        if (m_rightBlink)
        {
            m_pEyes->restoreRow(EyeMatrices::RIGHT, m_index);
            m_pEyes->restoreRow(EyeMatrices::RIGHT, 7-m_index);
        }
        m_pEyes->writeDisplays();

        startDelay(MILLISECONDS_FOR_BLINK_DELAY);

        m_index--;
        if (m_index < 0)
        {
            m_state = DONE;
            m_index = 0;
            m_isDone = true;
        }
        break;
    case DONE:
        assert ( m_state != DONE );
        break;
    }
}



void PupilAnimation::start(const PupilKeyFrame* pKeyFrames, size_t keyFrameCount)
{
    // Just return if nothing to do.
    if (keyFrameCount == 0)
    {
        m_isDone = true;
        return;
    }

    m_pFirstKeyFrame = pKeyFrames;
    m_pCurrKeyFrame = pKeyFrames;
    m_pLastKeyFrame = m_pFirstKeyFrame + keyFrameCount;

    startNextFrame();
    m_isDone = false;
    startDelay(0);
}

void PupilAnimation::startNextFrame()
{
    assert ( m_pCurrKeyFrame < m_pLastKeyFrame );

    // Start each eye's pupil out at its current position.
    m_startPos[EyeMatrices::LEFT] = m_pEyes->getPupilPos(EyeMatrices::LEFT);
    m_startPos[EyeMatrices::RIGHT] = m_pEyes->getPupilPos(EyeMatrices::RIGHT);

    // Target positions for each eye's pupil after fixup for out of range offsets.
    PupilPosition newPos[EyeMatrices::PUPIL_COUNT];
    newPos[EyeMatrices::LEFT] = EyeMatrices::getValidPupilPosition(&m_pCurrKeyFrame->left);
    newPos[EyeMatrices::RIGHT] = EyeMatrices::getValidPupilPosition(&m_pCurrKeyFrame->right);

    // The final step count for this animation will be the largest pixel traversal along any axis for either eye.
    PupilPosition steps[EyeMatrices::PUPIL_COUNT];
    int           perEyeSteps[EyeMatrices::PUPIL_COUNT];
    for (EyeMatrices::PupilEnum pupil = EyeMatrices::LEFT ;
         pupil <= EyeMatrices::RIGHT ;
         pupil = (EyeMatrices::PupilEnum)(pupil + 1))
    {
        // Determine how many pixels the pupil has to traverse along each axis.
        steps[pupil].x = abs(m_startPos[pupil].x - newPos[pupil].x);
        steps[pupil].y = abs(m_startPos[pupil].y - newPos[pupil].y);

        // The total number of steps to execute for the animation is determined by whether we need to interpolate or not.
        if (m_pCurrKeyFrame->interpolate)
        {
            // When interpolating, the steps required is the longest pixel distance along either axis.
            perEyeSteps[pupil] = (steps[pupil].x > steps[pupil].y) ? steps[pupil].x : steps[pupil].y;
        }
        else
        {
            // When not interpolating then the steps for the animation are always 1.
            perEyeSteps[pupil] = 1;
        }
    }
    m_steps = (perEyeSteps[EyeMatrices::LEFT] > perEyeSteps[EyeMatrices::RIGHT]) ?
              perEyeSteps[EyeMatrices::LEFT] : perEyeSteps[EyeMatrices::RIGHT];

    // Calculate the rest of the animation parameters now that the number of steps has been determined.
    for (EyeMatrices::PupilEnum pupil = EyeMatrices::LEFT ;
         pupil <= EyeMatrices::RIGHT ;
         pupil = (EyeMatrices::PupilEnum)(pupil + 1))
    {
        // Evaluate how much each axis should translate per step (floating point).
        m_changeX[pupil] = (float)steps[pupil].x / (float)m_steps;
        if (newPos[pupil].x < m_startPos[pupil].x)
            m_changeX[pupil] = -m_changeX[pupil];
        m_changeY[pupil] = (float)steps[pupil].y / (float)m_steps;
        if (newPos[pupil].y < m_startPos[pupil].y)
            m_changeY[pupil] = -m_changeY[pupil];
    }

    // Start at the first step.
    m_index = 1;

    // Remember the step delays for the current frame to use in run() method.
    m_frameDelay = m_pCurrKeyFrame->frameDelayStart;
    m_frameDelayStep = m_pCurrKeyFrame->frameDelayStep;
}

void PupilAnimation::run()
{
    // Just return if the animation is still waiting for a delay between frames or has completed all key frames.
    if (!isDelayDone() || isDone())
        return;

    assert ( m_index <= m_steps + 1 );
    if (m_index > m_steps)
    {
        // Have animated to the current key frame so advance to the next one.
        m_pCurrKeyFrame++;
        if (m_pCurrKeyFrame >= m_pLastKeyFrame)
        {
            // Have just finished the last key frame.
            m_isDone = true;
            return;
        }
        startNextFrame();
    }

    PupilPosition pupilPos[EyeMatrices::PUPIL_COUNT];
    for (EyeMatrices::PupilEnum pupil = EyeMatrices::LEFT ;
         pupil <= EyeMatrices::RIGHT ;
         pupil = (EyeMatrices::PupilEnum)(pupil + 1))
    {
        pupilPos[pupil].x = m_startPos[pupil].x + round(m_changeX[pupil] * (float)m_index);
        pupilPos[pupil].y = m_startPos[pupil].y + round(m_changeY[pupil] * (float)m_index);
    }
    m_pEyes->displayEyes(&pupilPos[EyeMatrices::LEFT], &pupilPos[EyeMatrices::RIGHT]);

    startDelay(m_frameDelay);
    m_frameDelay += m_frameDelayStep;

    m_index++;
}



void MoveEyeAnimation::start(int newX, int newY, uint32_t stepDelay)
{
    m_keyFrames[0].left.x = m_keyFrames[0].right.x = newX;
    m_keyFrames[0].left.y = m_keyFrames[0].right.y = newY;
    m_keyFrames[0].frameDelayStart = stepDelay;
    m_keyFrames[0].frameDelayStep = 0;
    m_keyFrames[0].interpolate = true;

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}



void CrossEyesAnimation::start()
{
    // Move eyes to center position first.
    m_keyFrames[0].left.x = m_keyFrames[0].right.x = 0;
    m_keyFrames[0].left.y = m_keyFrames[0].right.y = 0;
    m_keyFrames[0].frameDelayStart = 50;
    m_keyFrames[0].frameDelayStep = 0;
    m_keyFrames[0].interpolate = true;
    // Delay and stay at center position for half a second.
    m_keyFrames[1].left.x = m_keyFrames[1].right.x = 0;
    m_keyFrames[1].left.y = m_keyFrames[1].right.y = 0;
    m_keyFrames[1].frameDelayStart = 500;
    m_keyFrames[1].frameDelayStep = 0;
    m_keyFrames[1].interpolate = false;
    // Have each eye look in towards the nose.
    m_keyFrames[2].left.x = 2;
    m_keyFrames[2].left.y = 0;
    m_keyFrames[2].right.x = -2;
    m_keyFrames[2].right.y = 0;
    m_keyFrames[2].frameDelayStart = 100;
    m_keyFrames[2].frameDelayStep = 0;
    m_keyFrames[2].interpolate = true;
    // Delay and stay in crossed state for 2 seconds.
    m_keyFrames[3].left.x = 2;
    m_keyFrames[3].left.y = 0;
    m_keyFrames[3].right.x = -2;
    m_keyFrames[3].right.y = 0;
    m_keyFrames[3].frameDelayStart = 2000;
    m_keyFrames[3].frameDelayStep = 0;
    m_keyFrames[3].interpolate = false;
    // Move eyes out to center position again.
    m_keyFrames[4].left.x = m_keyFrames[4].right.x = 0;
    m_keyFrames[4].left.y = m_keyFrames[4].right.y = 0;
    m_keyFrames[4].frameDelayStart = 100;
    m_keyFrames[4].frameDelayStep = 0;
    m_keyFrames[4].interpolate = true;

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}



void RoundSpinAnimation::start()
{
    int i = 0;

    // Move eyes to center position first.
    m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
    m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
    m_keyFrames[i].frameDelayStart = 50;
    m_keyFrames[i].frameDelayStep = 0;
    m_keyFrames[i].interpolate = true;
    i++;

    // Delay and stay at center position for half a second.
    m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
    m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
    m_keyFrames[i].frameDelayStart = 500;
    m_keyFrames[i].frameDelayStep = 0;
    m_keyFrames[i].interpolate = false;
    i++;

    for (int j = 0 ; j < ROUND_SPIN_ITERATIONS; j++)
    {
        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = -1;
        m_keyFrames[i].frameDelayStart = 40 + ((j == 0) ? 40 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 1;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = -2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == 0) ? 30 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = -2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == 0) ? 20 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -1;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = -2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == 0) ? 10 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = -1;
        m_keyFrames[i].frameDelayStart = 40;
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
        m_keyFrames[i].frameDelayStart = 40;
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 1;
        m_keyFrames[i].frameDelayStart = 40 + ((j == ROUND_SPIN_ITERATIONS - 1) ? 10 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -1;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == ROUND_SPIN_ITERATIONS - 1) ? 20 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == ROUND_SPIN_ITERATIONS - 1) ? 30 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 1;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 2;
        m_keyFrames[i].frameDelayStart = 40 + ((j == ROUND_SPIN_ITERATIONS - 1) ? 40 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 1;
        m_keyFrames[i].frameDelayStart = 40 + ((j == ROUND_SPIN_ITERATIONS - 1) ? 50 : 0);
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 2;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
        m_keyFrames[i].frameDelayStart = 40;
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;
    }

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}




void CrazySpinAnimation::start()
{
    int i = 0;

    // Move eyes to center position first.
    m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
    m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
    m_keyFrames[i].frameDelayStart = 50;
    m_keyFrames[i].frameDelayStep = 0;
    m_keyFrames[i].interpolate = true;
    i++;

    // Delay and stay at center position for half a second.
    m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
    m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
    m_keyFrames[i].frameDelayStart = 500;
    m_keyFrames[i].frameDelayStep = 0;
    m_keyFrames[i].interpolate = false;
    i++;

    for (int j = 0 ; j < CRAZY_SPIN_ITERATIONS; j++)
    {
        // Scroll the pupil off screen to the left.
        // Start slow on first iteration and then accelerate to final speed.
        m_keyFrames[i].left.x = m_keyFrames[i].right.x = -5;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
        m_keyFrames[i].frameDelayStart = (j == 0) ? 100 : 50;
        m_keyFrames[i].frameDelayStep = (j == 0) ? -10 : 0;
        m_keyFrames[i].interpolate = true;
        i++;

        // Jump from left side off of screen to right side off of screen.
        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 5;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
        m_keyFrames[i].frameDelayStart = 0;
        m_keyFrames[i].frameDelayStep = 0;
        m_keyFrames[i].interpolate = false;
        i++;

        // Scroll the pupil from offscreen right to the center.
        // Decelerate the pupils on the last iteration.
        m_keyFrames[i].left.x = m_keyFrames[i].right.x = 0;
        m_keyFrames[i].left.y = m_keyFrames[i].right.y = 0;
        m_keyFrames[i].frameDelayStart = 50;
        m_keyFrames[i].frameDelayStep = (j == CRAZY_SPIN_ITERATIONS - 1) ? 10 : 0;
        m_keyFrames[i].interpolate = true;
        i++;
    }

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}



void MethEyesAnimation::start()
{
    // Move eyes to center position first.
    m_keyFrames[0].left.x = m_keyFrames[0].right.x = 0;
    m_keyFrames[0].left.y = m_keyFrames[0].right.y = 0;
    m_keyFrames[0].frameDelayStart = 50;
    m_keyFrames[0].frameDelayStep = 0;
    m_keyFrames[0].interpolate = true;
    // Delay and stay at center position for half a second.
    m_keyFrames[1].left.x = m_keyFrames[1].right.x = 0;
    m_keyFrames[1].left.y = m_keyFrames[1].right.y = 0;
    m_keyFrames[1].frameDelayStart = 500;
    m_keyFrames[1].frameDelayStep = 0;
    m_keyFrames[1].interpolate = false;
    // Have each eye look aways from the nose.
    m_keyFrames[2].left.x = -2;
    m_keyFrames[2].left.y = 0;
    m_keyFrames[2].right.x = 2;
    m_keyFrames[2].right.y = 0;
    m_keyFrames[2].frameDelayStart = 100;
    m_keyFrames[2].frameDelayStep = 0;
    m_keyFrames[2].interpolate = true;
    // Delay and stay in meth state for 2 seconds.
    m_keyFrames[3].left.x = -2;
    m_keyFrames[3].left.y = 0;
    m_keyFrames[3].right.x = 2;
    m_keyFrames[3].right.y = 0;
    m_keyFrames[3].frameDelayStart = 2000;
    m_keyFrames[3].frameDelayStep = 0;
    m_keyFrames[3].interpolate = false;
    // Move eyes out to center position again.
    m_keyFrames[4].left.x = m_keyFrames[4].right.x = 0;
    m_keyFrames[4].left.y = m_keyFrames[4].right.y = 0;
    m_keyFrames[4].frameDelayStart = 100;
    m_keyFrames[4].frameDelayStep = 0;
    m_keyFrames[4].interpolate = true;

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}



void LazyEyeAnimation::start()
{
    // Move eyes to look up a bit.
    m_keyFrames[0].left.x = m_keyFrames[0].right.x = 0;
    m_keyFrames[0].left.y = m_keyFrames[0].right.y = 1;
    m_keyFrames[0].frameDelayStart = 50;
    m_keyFrames[0].frameDelayStep = 0;
    m_keyFrames[0].interpolate = true;
    // Delay and stay at center position for half a second.
    m_keyFrames[1].left.x = m_keyFrames[1].right.x = 0;
    m_keyFrames[1].left.y = m_keyFrames[1].right.y = 1;
    m_keyFrames[1].frameDelayStart = 500;
    m_keyFrames[1].frameDelayStep = 0;
    m_keyFrames[1].interpolate = false;
    // Have right eye only look down slowly.
    m_keyFrames[2].left.x = 0;
    m_keyFrames[2].left.y = 1;
    m_keyFrames[2].right.x = 0;
    m_keyFrames[2].right.y = -2;
    m_keyFrames[2].frameDelayStart = 150;
    m_keyFrames[2].frameDelayStep = 0;
    m_keyFrames[2].interpolate = true;
    // Delay and stay in last state for 1 second2.
    m_keyFrames[3].left.x = 0;
    m_keyFrames[3].left.y = 1;
    m_keyFrames[3].right.x = 0;
    m_keyFrames[3].right.y = -2;
    m_keyFrames[3].frameDelayStart = 1000;
    m_keyFrames[3].frameDelayStep = 0;
    m_keyFrames[3].interpolate = false;
    // Move eyes out to center position again at a quick rate.
    m_keyFrames[4].left.x = m_keyFrames[4].right.x = 0;
    m_keyFrames[4].left.y = m_keyFrames[4].right.y = 1;
    m_keyFrames[4].frameDelayStart = 25;
    m_keyFrames[4].frameDelayStep = 0;
    m_keyFrames[4].interpolate = true;

    PupilAnimation::start(m_keyFrames, ARRAY_SIZE(m_keyFrames));
}



void GlowEyesAnimation::start(int iterations)
{
    m_iterations = iterations;

    // Start the eye(s) closing on the next call to run().
    m_isDone = false;
    m_state = BRIGHTER;
    m_brightness = 0;
    startDelay(0);
}

void GlowEyesAnimation::run()
{
    // Just return if the animation is still waiting for a delay between frames.
    if (!isDelayDone())
        return;

    int delay = 0;
    switch (m_state)
    {
    case BRIGHTER:
        m_pEyes->setBrightness(m_brightness);

        delay = 50;
        m_brightness++;
        if (m_brightness > 15)
        {
            m_state = DARKER;
            m_brightness = 15;
            delay += 250;
        }
        startDelay(delay);
        break;
    case DARKER:
        m_pEyes->setBrightness(m_brightness);

        delay = 25;
        m_brightness--;
        if (m_brightness < 0)
        {
            m_iterations--;
            if (m_iterations < 1)
            {
                m_state = DONE;
            }
            else
            {
                m_state = BRIGHTER;
            }

            m_brightness = 0;
            delay += 150;
        }
        startDelay(delay);
        break;
    case DONE:
        m_isDone = true;
        break;
    }
}
