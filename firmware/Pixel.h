/* Copyright (C) 2014 Leszek S   (http://stackoverflow.com/users/2102779/leszek-s)
   Copyright (C) 2016 Adam Green (https://github.com/adamgreen)

   Released under the Creative Commons Attribution Share-Alike 4.0 License
        https://creativecommons.org/licenses/by-sa/4.0/

    RGB <-> HSV conversion code was taken from a stack exchange answer posted by user "Leszek S"
        http://stackoverflow.com/a/14733008
*/
#ifndef PIXEL_H_
#define PIXEL_H_

#include <mbed.h>


struct RGBData
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    RGBData(int r, int g, int b) : red(r), green(g), blue(b)
    {
    }
    RGBData() : red(0), green(0), blue(0)
    {
    }
};

struct HSVData
{
    uint8_t hue;
    uint8_t saturation;
    uint8_t value;
};


// Commonly used colours.
#define RED         RGBData(0xFF, 0x00, 0x00)
#define ORANGE      RGBData(0xFF, 0xA5, 0x00)
#define DARK_ORANGE RGBData(0xFF, 0x8C, 0x00)
#define YELLOW      RGBData(0xFF, 0xFF, 0x00)
#define GREEN       RGBData(0x00, 0xFF, 0x00)
#define BLUE        RGBData(0x00, 0x00, 0xFF)
#define VIOLET      RGBData( 238,  130,  238)
#define BLACK       RGBData(0x00, 0x00, 0x00)
#define WHITE       RGBData(0xFF, 0xFF, 0xFF)


static inline void hsvToRgb(RGBData* pRGB, const HSVData* pHSV)
{
    uint32_t hue = pHSV->hue;
    uint32_t saturation = pHSV->saturation;
    uint32_t value = pHSV->value;

    if (saturation == 0)
    {
        pRGB->red = value;
        pRGB->green = value;
        pRGB->blue = value;
        return;
    }

    uint32_t region = hue / 43;
    uint32_t remainder = (hue - (region * 43)) * 6;

    uint32_t p = (value * (255 - saturation)) >> 8;
    uint32_t q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
    uint32_t t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
    case 0:
        pRGB->red = value;
        pRGB->green = t;
        pRGB->blue = p;
        break;
    case 1:
        pRGB->red = q;
        pRGB->green = value;
        pRGB->blue = p;
        break;
    case 2:
        pRGB->red = p;
        pRGB->green = value;
        pRGB->blue = t;
        break;
    case 3:
        pRGB->red = p;
        pRGB->green = q;
        pRGB->blue = value;
        break;
    case 4:
        pRGB->red = t;
        pRGB->green = p;
        pRGB->blue = value;
        break;
    default:
        pRGB->red = value;
        pRGB->green = p;
        pRGB->blue = q;
        break;
    }
}

static inline void rgbToHsv(HSVData* pHSV, const RGBData* pRGB)
{
    uint32_t red = pRGB->red;
    uint32_t green = pRGB->green;
    uint32_t blue = pRGB->blue;

    uint32_t rgbMin = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
    uint32_t rgbMax = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);

    pHSV->value = rgbMax;
    if (pHSV->value == 0)
    {
        pHSV->hue = 0;
        pHSV->saturation = 0;
    }

    pHSV->saturation = 255 * (rgbMax - rgbMin) / pHSV->value;
    if (pHSV->saturation == 0)
    {
        pHSV->hue = 0;
    }

    if (rgbMax == red)
    {
        pHSV->hue = 0 + 43 * (green - blue) / (rgbMax - rgbMin);
    }
    else if (rgbMax == green)
    {
        pHSV->hue = 85 + 43 * (blue - red) / (rgbMax - rgbMin);
    }
    else
    {
        pHSV->hue = 171 + 43 * (red - green) / (rgbMax - rgbMin);
    }
}

#endif // PIXEL_H_
