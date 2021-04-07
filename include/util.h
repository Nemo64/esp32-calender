#pragma once

#include <stddef.h>
#include <time.h>
#include <FreeRTOS.h>
#include <string.h>

template <typename T>
size_t addToSortedList(T* list, size_t& listSize, const size_t& maxListSize, const T& item, bool (*isGreaterThan)(const T&, const T&))
{
    for (size_t i = 0; i < listSize; ++i) {
        if (!isGreaterThan(list[i], item)) {
            continue;
        }

        if (listSize < maxListSize) {
            listSize++;
        }

        // move all entries (except the last entry) one further
        // if the listSize was increased beforehand, then _all_ existing entries are moved by 1
        // itemsToMove can be 0 if we replace the last value in list
        size_t itemsToMove = listSize - i - 1;
        memmove(&list[i + 1], &list[i], itemsToMove * sizeof(T));

        // now copy the actual value in it's proper place
        memcpy(&list[i], &item, sizeof(T));
        return i;
    }

    // if the above loop didn't add the item (and returned),
    // then append the item to the list if there is still space
    if (listSize < maxListSize) {
        memcpy(&list[listSize], &item, sizeof(T));
        return listSize++;
    }

    return maxListSize; // this is the position the item would have gotten if it were added
};

int inline calculateDaystamp(const tm& tm)
{
    int days = tm.tm_yday;
    days += (tm.tm_year - 70) * 365; // add days of year since 1970
    days += (tm.tm_year - 72) / 4; // add a day every 4 years since 1972 (first leap year since 1970)
    // ignore 100 year rules since i don't expect this code to run after 2100 and even if it did, it wouldn't really break
    //days -= (tm.tm_year - 100) / 100; // remove every 100th year based on 2000
    //days += (tm.tm_year - 100) / 400; // add every 400th year based on 2000
    return days;
}

int inline calculateDaystamp(const time_t& time)
{
    tm tm;
    localtime_r(&time, &tm);
    return calculateDaystamp(tm);
}

void executeOneTimeTask(void* parameters)
{
    auto func = (void (*)())parameters;
    func(); // call the passed function
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

void inline createAsyncOneTimeTask(const char* name, void(func)(), UBaseType_t priority = 0)
{
    xTaskCreate(executeOneTimeTask, name, CONFIG_ARDUINO_LOOP_STACK_SIZE, (void*)func, priority, nullptr);
}