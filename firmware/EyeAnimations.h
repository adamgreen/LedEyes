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



// Number of times the RoundSpinAnimation should spin the pupils.
#define ROUND_SPIN_ITERATIONS   2
// Number of times the CrazySpinAnimation should spin the pupils.
#define CRAZY_SPIN_ITERATIONS   2


// Used to indicate the position of the pupil in each eye. 0,0 places the pupil in the center of the eye. The limits
// of the offsets are -5 and 5 (EyeMatrices::PupilLimits). The x values increase as you progress to the right and the
// y values increase as you progress up. It should be noted that placing pupils at the limits (-5 & 5) will cause them
// to be rendered outside the matrix and therefore the pupil will disappear.
struct PupilPosition
{
    int x;
    int y;
};

// An array of PupilKeyFrame structures are passed into the PupilAnimation::start() method. They indicate how the
// pupil should be animated around within the eye.
struct PupilKeyFrame
{
    // Location of the left pupil (-5 to 5).
    PupilPosition left;
    // Location of the right pupil (-5 to 5).
    PupilPosition right;
    // The number of milliseconds to delay between steps (sub-frames) of the interpolation.
    uint32_t      frameDelayStart;
    // After each step in the animation, the frame delay will be increased by this amount (or decreased if negative).
    // This allows for acceleration to take place during the interpolated animation.
    int32_t       frameDelayStep;
    // Should the location of the pupil be interpolated and rendered as it moves from the previous location to the new
    // location or should it just jump to the new location.
    bool          interpolate;
};


// This class contains two instantiations of the Adafruit 8x8 matrices, one for each eye.
class EyeMatrices
{
public:
    // The limits of the offsets that can be used for the location of pupils. It should be noted that when placed at the
    // far limits, the pupils will disappear.
    enum PupilLimits
    {
        MIN = -5,
        MAX = 5
    };

    // There are two pupils: left and right.
    enum PupilEnum
    {
        LEFT = 0,
        RIGHT = 1,
        PUPIL_COUNT = 2
    };

    // Constructor - Initializes the 8x8 matrices on the I2C bus and sets the brightness to its lowest setting for both.
    //  pI2C is a pointer to the mbed I2C bus object to which both of the matrices have been attached.
    //  leftEyeAddress is the 7-bit I2C address of the 8x8 matrix to be used for the left eye.
    //  rightEyeAddress is the 7-bit I2C address of the 8x8 matrix to be used for the right eye.
    EyeMatrices(I2C* pI2C, uint8_t leftEyeAddress = 0x70, uint8_t rightEyeAddress = 0x71);

    // Initializes the eye matrices to draw the eyes with both pupils centered.
    void init()
    {
        PupilPosition pos = {0, 0};
        displayEyes(&pos, &pos);
    }

    // Display the eyes with each pupil located at the designated position.
    //  pLeftPos is a pointer to the struct representing the x,y coordinates of the left eye. (0,0) is centered,
    //           (-2,-2) is the lower left corner, and (2, 2) is the upper right corner.
    //  pRightPos is a pointer to the struct representing the x,y coordinates of the right eye. (0,0) is centered,
    //           (-2,-2) is the lower left corner, and (2, 2) is the upper right corner.
    void displayEyes(const PupilPosition* pLeftPos, const PupilPosition* pRightPos);

    // Temporarily turns off all pixels in a single row of the specified eye matrix.
    // This is useful for eye wink animations. The previous state of the row is still remembered and can be restored via
    // a call to the restoreRow() method.
    //  pupil specifies the eye to be updated. Allowed values are EyeMatrices::LEFT or EyeMatrices::RIGHT.
    //  row specifies the row for which all pixels should be temporarily turned off. Allowed values are between
    //      0 and 7.
    void turnRowOffTemporarily(PupilEnum pupil, int row);

    // Restores a row of pixels that have previously been turned off via a call to the turnRowOffTemporarily() method.
    //  pupil specifies the eye to be updated. Allowed values are EyeMatrices::LEFT or EyeMatrices::RIGHT.
    //  row specifies the row for which all pixels should be restored. Allowed values are between 0 and 7.
    void restoreRow(PupilEnum pupil, int row);

    // Sets the brightness of both eye matrices to the same value.
    //  brightness is the desired brightness. Allowed values are between 0 (BRIGHTNESS_MIN) and 15 (BRIGHTNESS_MAX).
    void setBrightness(uint8_t brightness)
    {
        left()->setBrightness(brightness);
        right()->setBrightness(brightness);
    }

    // Validates the x, y coordinates and caps them at the allowed [-5, 5] limits.
    //  pPos points to the x,y coordinates to be validated.
    //  Returns the x,y coordinates after they have both been limited to fall between -5 and 5 (inclusively).
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

    // Writes the display buffer out to the 8x8 matrices to be rendered.
    // Should be called after drawRow() has been used to manually update rows on the matrix displays. The displayEyes()
    // method calls this method internally so it doesn't need to be called again.
    void writeDisplays()
    {
        m_leftEye.writeDisplay();
        m_rightEye.writeDisplay();
    }

    // Returns the current value of the 32-bit millisecond counter.
    uint32_t getCurrentTime()
    {
        return m_timer.read_ms();
    }

    // Returns the current pupil position for the specified eye.
    //  pupil specifies which eye location is desired. Allowed values are EyeMatrices::LEFT or EyeMatrices::RIGHT.
    PupilPosition getPupilPos(PupilEnum pupil)
    {
        return m_currentPos[pupil];
    }


protected:
    // Returns a pointer to the driver object for the left 8x8 eye matrix.
    Adafruit_8x8matrix* left()
    {
        return &m_leftEye;
    }

    // Returns a pointer to the driver object for the right 8x8 eye matrix.
    Adafruit_8x8matrix* right()
    {
        return &m_rightEye;
    }

    // Returns a pointer to the driver object for the requested eye matrix.
    //  pupil specifies which matrix should be returned. Allowed values are LEFT or RIGHT.
    Adafruit_8x8matrix* eye(PupilEnum pupil)
    {
        switch (pupil)
        {
        case LEFT:
            return left();
            break;
        case RIGHT:
            return right();
            break;
        default:
            assert ( pupil == LEFT || pupil == RIGHT );
            return NULL;
        }
    }

    // Low level function for setting individual pixels on the specified row of a particular eye matrix.
    // Should call writeDisplays() later to have the row updates sent to the matrices to be rendered.
    //  pupil is either EyeMatrices::LEFT or EyeMatrices::RIGHT.
    //  row is the row of the 8x8 matrix to be updated. Allowed values are 0 to 7.
    //  rowData is an 8-bit value representing the state for each of the 8 pixels in the specified row. The least
    //          significant bit represents the leftmost pixel and the most significant bit represents the rightmost
    //          pixel. A bit value of 1 turns the pixel on and a value of 0 turns it off.
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

    uint8_t            m_eyeCurrent[PUPIL_COUNT][8];
    PupilPosition      m_currentPos[PUPIL_COUNT];
    Adafruit_8x8matrix m_leftEye;
    Adafruit_8x8matrix m_rightEye;
    Timer              m_timer;
};


// The virtual base class for all eye animation classes.
// It provides the virtual run()/isDone() method interface called from main().
// It also provides the millisecond delay timer required by most of the derived animations.
class EyeAnimationBase
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    EyeAnimationBase(EyeMatrices* pEyes)
    {
        m_pEyes = pEyes;
        m_startTime = 0;
        m_desiredDelay = 0;
    }

    // Virtual destructor.
    virtual ~EyeAnimationBase()
    {
    }

    // Starts a delay timer. isDelayDone() will return true once the timer has expired.
    //  desiredDelay specifies the delay time in milliseconds.
    void startDelay(uint32_t desiredDelay)
    {
        m_startTime = m_pEyes->getCurrentTime();
        m_desiredDelay = desiredDelay;
    }

    // Queries whether a delay timer set by startDelay() has expired yet or not.
    //  Returns true if the delay timer has expired and false otherwise.
    bool isDelayDone()
    {
        uint32_t delta = m_pEyes->getCurrentTime() - m_startTime;
        return delta >= m_desiredDelay;
    }


    // *** Virtual Methods to be implemented by all derived animation classes. ***

    // Called to give the animation CPU cycles to run its next step. It should just return immediately if the animation
    // currently has nothing to do yet (ie. it is still waiting for a delay timer to expire.) Each derived animation
    // will have its own start() method taking appropriate parameters to kick off its animation process.
    virtual void run() = 0;

    // Called to determine if this animation has run to completion.
    // Should return false while the animation is still in progress and then return true once it has completed.
    virtual bool isDone() = 0;

protected:
    EyeMatrices* m_pEyes;
    uint32_t     m_startTime;
    uint32_t     m_desiredDelay;
};


// This animation has the same interface [run()/isDone()] like the rest but it just inserts a delay into the main
// program flow and doesn't render anything.
class DelayAnimation : public EyeAnimationBase
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    DelayAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
    }

    // Virtual destructor.
    ~DelayAnimation()
    {
    }

    // Starts a delay animation which does nothing in the virtual run() method and returns true for isDone() only after
    // desiredDelay milliseconds has expired.
    void start(uint32_t desiredDelay)
    {
        startDelay(desiredDelay);
    }

    // Just returns while consuming a minimum of CPU cycles.
    virtual void run()
    {
    }

    // Returns true after the desired number of milliseconds have expired.
    virtual bool isDone()
    {
        return isDelayDone();
    }

protected:
};


// This animation blinks the eyes.
class BlinkAnimation : public EyeAnimationBase
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    BlinkAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }

    // Virtual destructor.
    ~BlinkAnimation()
    {
    }

    // Starts an animation which blinks the desired eyes(s).
    //  leftBlink should be set to true if you want the left eye to blink and false otherwise.
    //  rightBlink should be set to true if you want the right eye to blink and false otherwise.
    void start(bool leftBlink = true, bool rightBlink = true);

    // Run the blink animation code.
    virtual void run();

    // Returns true once the blink animation is completed and false otherwise.
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


// This is a generic animation class which plays through a caller supplied array of keyframes. Many of the pupil
// animations to follow will derive from this class and utilize this keyframe functionality for their animations.
class PupilAnimation : public EyeAnimationBase
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    PupilAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }

    // Virtual destructor.
    ~PupilAnimation()
    {
    }

    // Starts an animation controlled by the caller supplied keyframe array.
    //  pKeyFrames is a pointer to the array of keyframes to be rendered for this animation. See the documentation for
    //             the PupilKeyFrames structure to learn more about actions that can be performed as the animation
    //             progresses to each defined keyframe.
    //  keyFrameCount is the number of keyframe elements in the pKeyFrames array.
    void start(const PupilKeyFrame* pKeyFrames, size_t keyFrameCount);

    // Run the animation.
    virtual void run();

    // Returns true once all of the keyframes have been processed.
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
    float                m_changeX[EyeMatrices::PUPIL_COUNT];
    float                m_changeY[EyeMatrices::PUPIL_COUNT];
    int                  m_index;
    int                  m_steps;
    uint32_t             m_frameDelay;
    int32_t              m_frameDelayStep;
    bool                 m_isDone;
};


// This animation moves the pupils to the desired location.
class MoveEyeAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    MoveEyeAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~MoveEyeAnimation()
    {
    }

    // Starts the pupil animation to move the pupils for both eyes from their current location to the new location.
    //  newX is the horizontal offset to which both pupils should be moved. The accepted range is -5 to 5.
    //  newY is the vertical offset to which both pupils should be moved. The accepted range is -5 to 5.
    //  stepDelay is the time in milliseconds that the animation should delay between each interpolated frame as the
    //            pupils progress from their current location to their new location.
    void start(int newX, int newY, uint32_t stepDelay);

protected:
    PupilKeyFrame m_keyFrames[1];
};


// This animation causes both pupils to look inwards toward the nose. The animation starts and ends in the center
// position.
class CrossEyesAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    CrossEyesAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~CrossEyesAnimation()
    {
    }

    // Starts the cross eye pupil animation.
    void start();

protected:
    PupilKeyFrame m_keyFrames[5];
};


// This animation causes both pupils to rotate around the center of the eyes in a clockwise motion.
class RoundSpinAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    RoundSpinAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~RoundSpinAnimation()
    {
    }

    // Starts the pupils rotating around the eyes.
    void start();

protected:
    PupilKeyFrame m_keyFrames[2 + 12 * ROUND_SPIN_ITERATIONS];
};


// This animation scrolls the pupils horizontally across both eyes. It starts from the center and rotates to the far
// right, wraps around to the left side and then scrolls back into the center.
class CrazySpinAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    CrazySpinAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~CrazySpinAnimation()
    {
    }

    // Starts the pupil spinning.
    void start();

protected:
    PupilKeyFrame m_keyFrames[2 + 3 * CRAZY_SPIN_ITERATIONS];
};


// This animation causes both pupils to look outwards away from the nose. The animation starts and ends in the center
// position.
class MethEyesAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    MethEyesAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~MethEyesAnimation()
    {
    }

    // Starts the animation.
    void start();

protected:
    PupilKeyFrame m_keyFrames[5];
};


// This animation lowers the right eye to simulate a lazy eye.
class LazyEyeAnimation : public PupilAnimation
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    LazyEyeAnimation(EyeMatrices* pEyes) : PupilAnimation(pEyes)
    {
    }

    // Virtual destructor.
    ~LazyEyeAnimation()
    {
    }

    // Starts the lazy eye animation.
    void start();

protected:
    PupilKeyFrame m_keyFrames[5];
};


// This animation brightens the eyes with the pupil in their current position and then darkens them back down to their
// original brightness.
class GlowEyesAnimation : public EyeAnimationBase
{
public:
    // Constructor
    //  pEyes is a pointer to the EyeMatrices object to be used by the animations to render their eye animations.
    GlowEyesAnimation(EyeMatrices* pEyes) : EyeAnimationBase(pEyes)
    {
        m_isDone = true;
    }

    // Virtual destructor.
    ~GlowEyesAnimation()
    {
    }

    // Starts the brightness varying animation.
    //  iterations is the number of times to repeat the brighter/darker animation sequence.
    void start(int iterations);

    // Run the animation.
    virtual void run();

    // Returns true once the glow animation has completed and false before that.
    virtual bool isDone()
    {
        return m_isDone;
    }

protected:
    enum State
    {
        BRIGHTER,
        DARKER,
        DONE
    };

    int      m_brightness;
    int      m_iterations;
    State    m_state;
    bool     m_isDone;
};
