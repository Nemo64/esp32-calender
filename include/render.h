#pragma once

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/TomThumb.h>
#include <GxEPD2.h>
#include <time.h>

#include "dither.h"
#include "iCal.h"
#include "image.h"
#include "log.h"
#include "util.h"

const char* WEEK_DAYS[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };
const char* LONG_WEEK_DAYS[] = { "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" };
const char* MONTHS[] = { "Januar", "Februar", "Maerz", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" };

#define LARGE_FONT FreeSans24pt7b
#define LARGE_FONT_BOLD FreeSansBold24pt7b
const int16_t LARGE_LINE_HEIGHT = LARGE_FONT.yAdvance;
const int16_t LARGE_LINE_DISTANCE = LARGE_LINE_HEIGHT / 2;
const int16_t LARGE_PADDING = LARGE_LINE_DISTANCE / 4;

#define SMALL_FONT TomThumb
const int16_t SMALL_LINE_HEIGHT = SMALL_FONT.yAdvance + 2;
const int16_t SMALL_LINE_DISTANCE = SMALL_LINE_HEIGHT / 2;
const int16_t SMALL_PADDING = 2;

void renderTextCentered(Adafruit_GFX& canvas, uint16_t cw, const char* message)
{
    int16_t bx, by;
    uint16_t tw, th;
    canvas.getTextBounds(message, canvas.getCursorX(), canvas.getCursorY(), &bx, &by, &tw, &th);
    canvas.setCursor(canvas.getCursorX() + cw / 2 - tw / 2, canvas.getCursorY());
    canvas.print(message);
}

void renderCalender(Adafruit_GFX& canvas, time_t timestamp, ICalEntry* entries, size_t size)
{
    canvas.setTextWrap(false);
    canvas.setFont(&LARGE_FONT); // set the font before the first cursor set to avoid the 6px move by switching between font types
    canvas.setCursor(LARGE_PADDING, LARGE_LINE_HEIGHT - LARGE_LINE_DISTANCE / 2);

    tm currentTime;
    localtime_r(&timestamp, &currentTime);
    int currentDay = calculateDaystamp(currentTime);
    int lastDay = 0;
    for (size_t i = 0; i < size; ++i) {
        tm entryTime;
        localtime_r(&entries[i].start, &entryTime);
        int entryDay = calculateDaystamp(entryTime);
        int dayOffset = entryDay - currentDay;
        LOGI("render", "render day offset %d with entry day %d", dayOffset, entryDay);

        // draw header
        if (lastDay != entryDay) {
            lastDay = entryDay;

            if (i > 0) {
                canvas.setCursor(LARGE_PADDING, canvas.getCursorY() + LARGE_LINE_DISTANCE / 2);
            }

            canvas.setTextColor(GxEPD_WHITE);
            canvas.setFont(&LARGE_FONT_BOLD);

            xy_t headerPos = { 0, (int16_t)(canvas.getCursorY() + LARGE_LINE_DISTANCE / 2 - LARGE_LINE_HEIGHT) };
            xy_t headerDim = { canvas.width(), LARGE_LINE_HEIGHT };
            if (dayOffset == 0) {
                drawGradientX(canvas, headerPos, headerDim, COLORSPACE_3C, GxEPD_BLACK, GxEPD_RED);
                canvas.print("Heute");
            } else if (dayOffset == 1) {
                drawGradientX(canvas, headerPos, headerDim, COLORSPACE_3C, GxEPD_BLACK, GxEPD_RED);
                canvas.print("Morgen");
            } else if (dayOffset <= 3) {
                drawGradientX(canvas, headerPos, headerDim, COLORSPACE_3C, GxEPD_BLACK, GxEPD_RED);
                canvas.print(LONG_WEEK_DAYS[entryTime.tm_wday]);
            } else {
                drawGradientX(canvas, headerPos, headerDim, COLORSPACE_2C, GxEPD_BLACK, mix(GxEPD_BLACK, GxEPD_WHITE, 128));
                canvas.printf("%s %02d. %s", WEEK_DAYS[entryTime.tm_wday], entryTime.tm_mday, MONTHS[entryTime.tm_mon]);
            }

            canvas.setCursor(LARGE_PADDING, canvas.getCursorY() + LARGE_LINE_HEIGHT);
        }

        canvas.setTextColor(GxEPD_BLACK);
        canvas.setFont(&LARGE_FONT);
        canvas.print(entries[i].summary);

        canvas.setCursor(LARGE_PADDING, canvas.getCursorY() + LARGE_LINE_HEIGHT);
        if (canvas.getCursorY() > canvas.height() + LARGE_LINE_HEIGHT) {
            break;
        }
    }

    loopRect({ 0, (int16_t)(canvas.height() - 64) }, { canvas.width(), 64 }, [&canvas](xy_t relPos, xy_t pos) {
        auto color = dither(pos, COLORSPACE_2C, mix(GxEPD_BLACK, GxEPD_WHITE, relPos.y * 255 / 95));
        if (color == GxEPD_WHITE) {
            canvas.drawPixel(pos.x, pos.y, GxEPD_WHITE);
        }
    });
}

void renderError(Adafruit_GFX& canvas, const char* title, const char* message)
{
    canvas.drawBitmap(
        canvas.width() / 2 - exclamation.width / 2,
        canvas.height() / 2 - exclamation.height,
        exclamationBitmap,
        exclamation.width,
        exclamation.height,
        GxEPD_RED);

    canvas.setFont(&LARGE_FONT);
    canvas.setTextColor(GxEPD_BLACK);
    canvas.setCursor(0, canvas.height() / 2 + LARGE_LINE_HEIGHT);
    canvas.setTextWrap(false);
    renderTextCentered(canvas, canvas.width(), title);

    canvas.setFont(&SMALL_FONT);
    canvas.setTextWrap(true);
    canvas.setCursor(0, canvas.getCursorY() + LARGE_LINE_DISTANCE + SMALL_FONT.yAdvance);
    renderTextCentered(canvas, canvas.width(), message);
}

void renderFooter(Adafruit_GFX& canvas, time_t timestamp, unsigned sleepTime, unsigned voltage)
{
    tm currentTime;
    localtime_r(&timestamp, &currentTime);

    canvas.setFont(&SMALL_FONT);
    canvas.setTextColor(GxEPD_BLACK);
    canvas.fillRect(0, canvas.height() - SMALL_LINE_HEIGHT, canvas.width(), SMALL_LINE_HEIGHT, GxEPD_WHITE);
    canvas.setCursor(SMALL_PADDING, canvas.height() - (SMALL_LINE_HEIGHT - SMALL_FONT.yAdvance));
    canvas.print("aktuallisiert ");
    canvas.printf("%s %02d. %s %04d %02d:%02d:%02d",
        LONG_WEEK_DAYS[currentTime.tm_wday],
        currentTime.tm_mday,
        MONTHS[currentTime.tm_mon],
        currentTime.tm_year + 1900,
        currentTime.tm_hour,
        currentTime.tm_min,
        currentTime.tm_sec);

    canvas.setTextColor(sleepTime < (3600 * 4) ? GxEPD_RED : GxEPD_BLACK);
    canvas.printf(", naechstes %u:%02u:%02u", sleepTime / 3600, sleepTime / 60 % 60, sleepTime % 60);

    if (voltage > 0) {
        canvas.setTextColor(voltage < 3200 ? GxEPD_RED : GxEPD_BLACK);
        canvas.printf(", %u.%03u V", voltage / 1000, voltage % 1000);
    }
}
