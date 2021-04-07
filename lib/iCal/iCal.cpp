#include <time.h>
#include <Stream.h>
#include <cstdio>
#include "iCal.h"

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

ICalResult readICalEntry(Stream* stream, ICalEntry* target)
{
    bool inCalenderEntry = false;
    char line[128];
    tm time;

    bool hasSummary = false;
    bool hasDate = false;

    while (auto lineLength = stream->readBytesUntil('\n', line, sizeof(line) - 1)) {
        line[lineLength] = '\0'; // ensure the 0 byte is there
        if (lineLength == 0) {
            continue;
        }

        if (!inCalenderEntry) {
            if (strncmp(line, "END:VCALENDAR", 13) == 0) {
                return ICAL_END;
            }

            if (strncmp(line, "BEGIN:VEVENT", 12) == 0) {
                inCalenderEntry = true;
            }

            continue;
        }

        if (sscanf(line, "SUMMARY:%20[^\r\n(]", target->summary)) {
            hasSummary = true;
            continue;
        }

        if (sscanf(line, "DTSTART;VALUE=DATE:20%2d%2d%2d", &time.tm_year, &time.tm_mon, &time.tm_mday)) {
            time.tm_year += 100;
            time.tm_mon -= 1;
            time.tm_hour = 0;
            time.tm_min = 0;
            time.tm_sec = 0;
            time.tm_isdst = -1;
            target->start = mktime(&time);
            hasDate = true;
            continue;
        }

        if (strncmp(line, "END:VEVENT", 10) == 0) {
            if (hasSummary && hasDate) {
                return ICAL_OK;
            }

            hasSummary = false;
            hasDate = false;
            continue;
        }
    };

    return ICAL_END_UNEXPECRED;
}

ICalResult readICalStream(Stream* stream, ICalEntry* list, size_t& listSize, size_t maxSize, time_t startTime)
{
    ICalEntry readEntry;
    ICalResult lastResult;
    
    unsigned long count = 0;
    while ((lastResult = readICalEntry(stream, &readEntry)) == ICAL_OK) {
        if (readEntry.start >= startTime) {
            addToSortedList<ICalEntry>(list, listSize, maxSize, readEntry, [](const ICalEntry& a, const ICalEntry& b) {
                return a.start > b.start;
            });
        }
    }

    return lastResult;
}
