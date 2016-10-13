/***************************************************
  This is a library for our I2C LED Backpacks

  Designed specifically to work with the Adafruit LED Matrix backpacks
  ----> http://www.adafruit.com/products/
  ----> http://www.adafruit.com/products/

  These displays use I2C to communicate, 2 pins are required to
  interface. There are multiple selectable I2C addresses. For backpacks
  with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
  with 3 Address Select pins: 0x70 thru 0x77

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

  Adam Green: I ported this code to run on mbed but since I only
              had the 8x8 matrix, that is all I ported and tested.
 ****************************************************/
#ifndef Adafruit_LEDBackpack_h
#define Adafruit_LEDBackpack_h

#include <stdint.h>
#include <mbed.h>


#define LED_ON  true
#define LED_OFF false

// Minimum and maximum brightness values that can be used with Adafruit_LEDBackpack::setBrightness() method.
#define BRIGHTNESS_MAX 15
#define BRIGHTNESS_MIN 0

// Blink rates that can be used with Adafruit_LEDBackpack::blinkRate() method.
#define HT16K33_BLINK_OFF           0
#define HT16K33_BLINK_2HZ           1
#define HT16K33_BLINK_1HZ           2
#define HT16K33_BLINK_HALFHZ        3


// this is the raw HT16K33 controller
class Adafruit_LEDBackpack
{
public:
    Adafruit_LEDBackpack(PinName sda, PinName scl)  : m_i2c(sda, scl)
    {
        clear();
    }

    // Should be first call to backpack. Initializes the backpack, sets to maximum brightness, and makes sure that
    // blinking is disabled.
    void begin(uint8_t i2cAddress = 0x70);
    // Used to set the brightness of all LEDs that are on. Can't be used to individually set the brightness of LEDs.
    // Valid values for brightness are between 0 and 15 (BRIGHTNESS_MIN and BRIGHTNESS_MAX).
    void setBrightness(uint8_t brightness);
    // Set the blink rate. Allowed values are HT16K33_BLINK_OFF, HT16K33_BLINK_2HZ, HT16K33_BLINK_1HZ, or
    // HT16K33_BLINK_HALFHZ.
    void blinkRate(uint8_t rate);
    // Display the current contents of the display buffer to the RAM of the HT16K33.
    void writeDisplay(void);
    // Clears out the display buffer. An immediate call to writeDisplay() after this would turn off all of the LEDs.
    void clear(void);

    // The HTK16K33 has a RAM which can hold 8 rows of 16 columns (2 bytes per row).
    // Allocate an extra byte to hold initial write command byte so that everything can be sent out in one I2C write
    // transaction.
    uint8_t m_displayBuffer[8 * 2 + 1];

protected:
    I2C     m_i2c;
    uint8_t m_i2cAddress;
};


class Adafruit_8x8matrix : public Adafruit_LEDBackpack
{
public:
    Adafruit_8x8matrix(PinName sda, PinName scl) : Adafruit_LEDBackpack(sda, scl)
    {
    }

    // Sets or clears (based on color boolean parameter) the pixel in the display buffer at the desired x/y coordinate.
    // Still need a subsequent call to writeDisplay() to actually send the pixel data to the interal RAM of the HT16K33.
    void drawPixel(int x, int y, bool color);
};

#endif // Adafruit_LEDBackpack_h

