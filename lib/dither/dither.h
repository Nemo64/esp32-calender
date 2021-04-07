#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

typedef uint16_t color_t;
static const color_t COLORSPACE_2C[] = { 0x0000, 0xFFFF };
static const color_t COLORSPACE_3C[] = { 0x0000, 0xF800, 0xFFFF };

struct xy_t {
    int16_t x;
    int16_t y;

    xy_t inline operator+(const xy_t& other)
    {
        return {
            (int16_t)(this->x + other.x),
            (int16_t)(this->y + other.y)
        };
    }

    xy_t inline operator-(const xy_t& other)
    {
        return {
            (int16_t)(this->x - other.x),
            (int16_t)(this->y - other.y)
        };
    }
};

/**
 * This function mixes the two provided colors by the provided ratio.
 * The ratio is from 0 - 256 where 0 is exactly c1 and 256 is exactly c2.
 * However, the max ratio you can provide is 255 so your logic must handle this case.
 */
color_t mix(color_t c1, color_t c2, uint8_t ratio);

/**
 * This function handles pattern dithering.
 * Just provide a color and a palette and it'll give you a color for that particular pixel.
 * You have to call if for each pixel for best results.
 */
color_t dither(xy_t pos, const color_t* palette, color_t color);

/**
 * This utility allows you to easily loop over a rectangle.
 * Just provide the position and size and your provided function will get a relative and an absolute position.
 */
template <typename F>
void inline loopRect(xy_t pos, xy_t size, F func)
{
    xy_t abs;
    for (abs.x = pos.x; abs.x < pos.x + size.x; ++abs.x) {
        for (abs.y = pos.y; abs.y < pos.y + size.y; ++abs.y) {
            func(abs - pos, abs);
        }
    }
}

void drawRect(Adafruit_GFX& canvas, xy_t pos, xy_t dim, const color_t* palette, color_t color);
void drawGradientX(Adafruit_GFX& canvas, xy_t pos, xy_t dim, const color_t* palette, color_t c1, color_t c2);
void drawGradientY(Adafruit_GFX& canvas, xy_t pos, xy_t dim, const color_t* palette, color_t c1, color_t c2);