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

#include "Adafruit_LEDBackpack.h"

// HTK16K33 I2C commands.
#define HT16K33_CMD_BLINK           0x80
#define HT16K33_BLINK_DISPLAYON     0x01
#define HT16K33_CMD_BRIGHTNESS      0xE0
#define HT16K33_CMD_OSCILLATOR_ON   0x21
#define HT16K33_CMD_OSCILLATOR_OFF  0x20


void Adafruit_LEDBackpack::setBrightness(uint8_t brightness)
{
    if (brightness > 15)
        brightness = 15;

    char command = HT16K33_CMD_BRIGHTNESS | brightness;
    m_i2c.write(m_i2cAddress, &command, sizeof(command));
}

void Adafruit_LEDBackpack::blinkRate(uint8_t rate)
{
    // turn off if not sure.
    if (rate > HT16K33_BLINK_HALFHZ)
        rate = HT16K33_BLINK_OFF;

    char command = HT16K33_CMD_BLINK | HT16K33_BLINK_DISPLAYON | (rate << 1);
    m_i2c.write(m_i2cAddress, &command, sizeof(command));
}

void Adafruit_LEDBackpack::begin(uint8_t i2cAddress /* = 0x70 */)
{
    // Left shift the address by one so that we can use the same address as
    // Adafruit quotes in their documentation with the mbed SDK since Adafruit
    // uses the 7-bit address and mbed uses the 8-bit address.
    m_i2cAddress = i2cAddress << 1;

    char command = HT16K33_CMD_OSCILLATOR_ON;
    m_i2c.write(m_i2cAddress, &command, sizeof(command));

    blinkRate(HT16K33_BLINK_OFF);

    setBrightness(BRIGHTNESS_MAX);
}

void Adafruit_LEDBackpack::writeDisplay(void)
{
    // First byte of display buffer was reserved for address command.
    // Start at address 0.
    m_displayBuffer[0] = 0x00;

    m_i2c.write(m_i2cAddress, (const char*)m_displayBuffer, sizeof(m_displayBuffer));
}

void Adafruit_LEDBackpack::clear(void)
{
    memset(m_displayBuffer, 0, sizeof(m_displayBuffer));
}



/******************************* 8x8 MATRIX OBJECT */
void Adafruit_8x8matrix::drawPixel(int x, int y, bool color)
{
    if ((y < 0) || (y >= 8))
        return;
    if ((x < 0) || (x >= 8))
        return;

    // The pixels are stored with 0th pixel in lsb and then the byte rotated right by 1.
    x = (x + 7) & 7;

    // The column bitmasks are sent in every other byte plus the first byte is reserved for the command.
    if (color)
        m_displayBuffer[y * 2 + 1] |= (1 << x);
    else
        m_displayBuffer[y * 2 + 1] &= ~(1 << x);
}
