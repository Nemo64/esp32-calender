#pragma once

#include <time.h>

struct ICalEntry {
    time_t start;
    char summary[20];
};

enum ICalResult {
    ICAL_OK,
    ICAL_END,
    ICAL_END_UNEXPECRED,
};

/**
 * Reads all iCal entries from the given stream into the given array in ascending order.
 * All items before startTime are dropped.
 * Items that don't fit in the list are dropped as well.
 */
ICalResult readICalStream(Stream* stream, ICalEntry* list, size_t& listSize, size_t maxSize, time_t startTime);
