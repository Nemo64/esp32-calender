#include "dither.h"

#define EXT_RED(color) (((color) >> 10) & 0b111110)
#define EXT_GREEN(color) (((color) >> 5) & 0b111111)
#define EXT_BLUE(color) (((color) << 1) & 0b111110)

#define PACK_RED(color) (((color)&0b111110) << 10)
#define PACK_GREEN(color) (((color)&0b111111) << 5)
#define PACK_BLUE(color) (((color)&0b111110) >> 1)

// https://en.wikipedia.org/wiki/Ordered_dithering
#define DITHER_MAX 63
static const int8_t DITHER_PATTERN[8][8] = {
    { 0, 48, 12, 60, 3, 51, 15, 63 },
    { 32, 16, 44, 28, 35, 19, 47, 31 },
    { 8, 56, 4, 52, 11, 59, 7, 55 },
    { 40, 24, 36, 20, 43, 27, 39, 23 },
    { 2, 50, 14, 62, 1, 49, 13, 61 },
    { 34, 18, 46, 30, 33, 17, 45, 29 },
    { 10, 58, 6, 54, 9, 57, 5, 53 },
    { 42, 26, 38, 22, 41, 25, 37, 21 },
};

uint16_t brightness(color_t color)
{
    return EXT_RED(color) * 3 + EXT_GREEN(color) * 6 + EXT_BLUE(color);
}

color_t mix(color_t c1, color_t c2, uint8_t ratio)
{
    return PACK_RED(EXT_RED(c1) * ratio / 256 + EXT_RED(c2) * ratio / 256)
        | PACK_GREEN(EXT_GREEN(c1) * ratio / 256 + EXT_GREEN(c2) * ratio / 256)
        | PACK_BLUE(EXT_BLUE(c1) * ratio / 256 + EXT_BLUE(c2) * ratio / 256);
}

color_t dither(xy_t pos, const color_t* palette, color_t color)
{
    uint16_t lowDistance = 0xFFFF;
    color_t lowColor = 0x0000;
    uint16_t highDistance = 0xFFFF;
    color_t highColor = 0xFFFF;

    // this algorythm is not great at all since it only respects brightness
    // but all other algorythms are overkill for 3 colors

    size_t i = 0;
    auto colorBrightness = brightness(color);
    do {
        auto brightnessDistance = brightness(palette[i]) - colorBrightness;
        if (brightnessDistance == 0) {
            return palette[i];
        } else if (brightnessDistance > 0 && highDistance > brightnessDistance) {
            highDistance = brightnessDistance;
            highColor = palette[i];
        } else if (brightnessDistance < 0 && lowDistance > -brightnessDistance) {
            lowDistance = -brightnessDistance;
            lowColor = palette[i];
        }
    } while (palette[i++] != 0xFFFF);

    auto threshold = lowDistance * DITHER_MAX / (lowDistance + highDistance);
    return DITHER_PATTERN[pos.x % 8][pos.y % 8] > threshold ? lowColor : highColor;
}

void drawRect(Adafruit_GFX& canvas, xy_t pos, xy_t size, const color_t* palette, color_t color)
{
    loopRect(pos, size, [&canvas, &palette, &color](xy_t rel, xy_t abs) {
        canvas.drawPixel(abs.x, abs.y, dither(abs, palette, color));
    });
}

void drawGradientX(Adafruit_GFX& canvas, xy_t pos, xy_t size, const color_t* palette, color_t c1, color_t c2)
{
    loopRect(pos, size, [&](xy_t rel, xy_t abs) {
        auto color = rel.x * 255 / (size.x - 1);
        canvas.drawPixel(abs.x, abs.y, dither(abs, palette, mix(c1, c2, color)));
    });
}

void drawGradientY(Adafruit_GFX& canvas, xy_t pos, xy_t size, const color_t* palette, color_t c1, color_t c2)
{
    loopRect(pos, size, [&](xy_t rel, xy_t abs) {
        auto color = rel.y * 255 / (size.y - 1);
        canvas.drawPixel(abs.x, abs.y, dither(abs, palette, mix(c1, c2, color)));
    });
}
