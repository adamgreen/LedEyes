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
    m_rightEye.begin(rightEyeAddress);
    m_timer.start();
}

void EyeMatrices::displayEyes(int offsetX, int offsetY)
{
    // ensure offsets are in valid ranges
    offsetX = getValidValue(offsetX);
    offsetY = getValidValue(offsetY);

    // calculate indexes for pupil rows (perform offset Y)
    int row1 = 3 - offsetY;
    int row2 = 4 - offsetY;

    // define pupil row
    uint8_t pupilRow = g_eyePupil;

    // UNDONE: Can probably be done without a loop.
    // perform offset X
    // bit shift and fill in new bit with 1
    if (offsetX > 0)
    {
        for (int i=1; i<=offsetX; i++)
        {
            pupilRow = pupilRow >> 1;
            pupilRow = pupilRow | 0x80;
        }
    }
    else if (offsetX < 0)
    {
        for (int i=-1; i>=offsetX; i--)
        {
            pupilRow = pupilRow << 1;
            pupilRow = pupilRow | 0x01;
        }
    }

    // pupil row cannot have 1s where eyeBall has 0s
    uint8_t pupilRow1 = pupilRow & g_eyeBall[row1];
    uint8_t pupilRow2 = pupilRow & g_eyeBall[row2];

    // display on LCD matrix, update to eyeCurrent
    for(int r=0; r<8; r++)
    {
        if (r == row1)
        {
            m_leftEye.drawRow(r, pupilRow1);
            m_rightEye.drawRow(r, pupilRow1);
            m_eyeCurrent[r] = pupilRow1;
        }
        else if (r == row2)
        {
            m_leftEye.drawRow(r, pupilRow2);
            m_rightEye.drawRow(r, pupilRow2);
            m_eyeCurrent[r] = pupilRow2;
        }
        else
        {
            m_leftEye.drawRow(r, g_eyeBall[r]);
            m_rightEye.drawRow(r, g_eyeBall[r]);
            m_eyeCurrent[r] = g_eyeBall[r];
        }
    }

    // update current X and Y
    m_currentX = offsetX;
    m_currentY = offsetY;

    writeDisplays();
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
            m_pEyes->left()->drawRow(m_index, 0);
            m_pEyes->left()->drawRow(7-m_index, 0);
        }
        if (m_rightBlink)
        {
            m_pEyes->right()->drawRow(m_index, 0);
            m_pEyes->right()->drawRow(7-m_index, 0);
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
            m_pEyes->left()->drawRow(m_index, m_pEyes->m_eyeCurrent[m_index]);
            m_pEyes->left()->drawRow(7-m_index, m_pEyes->m_eyeCurrent[7-m_index]);
        }
        if (m_rightBlink)
        {
            m_pEyes->right()->drawRow(m_index, m_pEyes->m_eyeCurrent[m_index]);
            m_pEyes->right()->drawRow(7-m_index, m_pEyes->m_eyeCurrent[7-m_index]);
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



void MoveEyeAnimation::start(int newX, int newY, uint32_t stepDelay)
{
    // set current position as start position
    m_startX = m_pEyes->m_currentX;
    m_startY = m_pEyes->m_currentY;

    // fix invalid new X Y values
    newX = EyeMatrices::getValidValue(newX);
    newY = EyeMatrices::getValidValue(newY);

    // eval steps
    int stepsX = abs(m_pEyes->m_currentX - newX);
    int stepsY = abs(m_pEyes->m_currentY - newY);

    // need to change at least one position
    if ((stepsX == 0) && (stepsY == 0))
    {
        m_isDone = true;
        return;
    }

    // eval direction of movement, # of steps, change per X Y step, perform move
    m_dirX = (newX >= m_pEyes->m_currentX) ? 1 : -1;
    m_dirY = (newY >= m_pEyes->m_currentY) ? 1 : -1;
    m_steps = (stepsX > stepsY) ? stepsX : stepsY;
    m_changeX = (float)stepsX / (float)m_steps;
    m_changeY = (float)stepsY / (float)m_steps;
    m_stepDelay = stepDelay;
    m_index = 1;

    m_isDone = false;
    startDelay(0);
}

void MoveEyeAnimation::run()
{
    // Just return if the animation is still waiting for a delay between frames.
    if (!isDelayDone())
        return;

    // Move eye to next offset on its path to final destination.
    int intX = m_startX + round(m_changeX * m_index * m_dirX);
    int intY = m_startY + round(m_changeY * m_index * m_dirY);
    m_pEyes->displayEyes(intX, intY);

    startDelay(m_stepDelay);

    m_index++;
    if (m_index > m_steps)
    {
        m_isDone = true;
    }
}
