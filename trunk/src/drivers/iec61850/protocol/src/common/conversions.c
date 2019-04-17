/*
 *  conversions.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "libiec61850_platform_includes.h"
#include "conversions.h"

#include <time.h>

#if defined TARGET
#if (TARGET == UCLINUX-WAGO)
time_t timegm (struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv ("TZ");
    setenv ("TZ", "", 1);
    tzset ();
    ret = mktime (tm);
    if (tz)
        setenv ("TZ", tz, 1);
    else
        unsetenv ("TZ");
    tzset ();
    return ret;
}

#endif
#endif

#ifdef _WIN32

time_t
timegm(struct tm* tm_time)
{
    return mktime(tm_time) - _timezone;
}

#if defined(__MINGW32__)

static inline // assuming gmtime is thread safe in windows!
struct tm* gmtime_r(const time_t* timep, struct tm* result)
{
   struct tm* t;

   t = gmtime(timep);

   if (t != NULL)
       memcpy(result, t, sizeof (struct tm));

   return result;
}

#else

#if defined(_MSC_VER)

static __inline
struct tm* gmtime_r(const time_t* timep, struct tm* result)
{
    //gmtime_s(result, timep);
	result = gmtime(timep);

    return result;
}

#else
#error "No gmtime_r available for platform!"
#endif


#endif


#endif

void
Conversions_intToStringBuffer(int intValue, int numberOfDigits, uint8_t* buffer)
{
    int digitBase = 1;

    int i = 1;

	int remainder = intValue;

    while (i < numberOfDigits) {
        digitBase = digitBase * 10;
        i++;
    }

    for (i = 0; i < numberOfDigits; i++) {
        int digit = remainder / digitBase;

        buffer[i] = (uint8_t) (digit + 48);

        remainder = remainder % digitBase;

        digitBase = digitBase / 10;
    }

    buffer[i] = 0;
}

void
Conversions_msTimeToGeneralizedTime(uint64_t msTime, uint8_t* buffer)
{
    int msPart = (msTime % 1000);

    time_t unixTime = (msTime / 1000);

    struct tm tmTime;

    gmtime_r(&unixTime, &tmTime);

    Conversions_intToStringBuffer(tmTime.tm_year + 1900, 4, buffer);

    Conversions_intToStringBuffer(tmTime.tm_mon + 1, 2, buffer + 4);
    Conversions_intToStringBuffer(tmTime.tm_mday, 2, buffer + 6);
    Conversions_intToStringBuffer(tmTime.tm_hour, 2, buffer + 8);
    Conversions_intToStringBuffer(tmTime.tm_min, 2, buffer + 10);
    Conversions_intToStringBuffer(tmTime.tm_sec, 2, buffer + 12);

    buffer[14] = '.';

    Conversions_intToStringBuffer(msPart, 3, buffer + 15);

    buffer[18] = 'Z';

    buffer[19] = 0;
}

static int
getSecondsOffset(const char* offsetString)
{
    int hourOffset = StringUtils_digitsToInt(offsetString, 2);
	int minOffset;
	int secondsOffset;

    if (hourOffset < 0)
        return -1;

    minOffset = StringUtils_digitsToInt(offsetString + 2, 2);

    if (minOffset < 0)
        return -1;

    secondsOffset = (hourOffset * (60 * 60)) + (minOffset * 60);

    return secondsOffset;
}

uint64_t
Conversions_generalizedTimeToMsTime(const char* gtString)
{
    int gtStringLen = strlen(gtString);
	int year;
	int month;
	int day;
	int hour;
	int min;
	int seconds;
	struct tm tmTime;
	int msOffset;
	char* parsePos;
	time_t t;
	uint64_t msTime;

    if (gtStringLen < 14) return -1;

    year = StringUtils_digitsToInt(gtString, 4);

    if (year < 0) return -1;

    month = StringUtils_digitsToInt(gtString + 4, 2);

    if (month < 0) return -1;

    day = StringUtils_digitsToInt(gtString + 6, 2);

    if (day < 0) return -1;

    hour = StringUtils_digitsToInt(gtString + 8, 2);

    if (hour < 0) return -1;

    min = StringUtils_digitsToInt(gtString + 10, 2);

    if (min < 0) return -1;

    seconds = StringUtils_digitsToInt(gtString + 12, 2);
    if (seconds < 0) return -1;
    
    tmTime.tm_year = year - 1900;
    tmTime.tm_mon = month - 1;
    tmTime.tm_mday = day;
    tmTime.tm_hour = hour;
    tmTime.tm_min = min;
    tmTime.tm_sec = seconds;

    msOffset = 0;

    parsePos = gtString + 14;

    /* parse optional fraction of second field */
    if (*(parsePos) == '.') {
        
        const char* fractionOfSecondStart = parsePos;

        int fractionOfSecondLen = 0;

        int secondValue = 1;

		parsePos++;

        while (StringUtils_isDigit(fractionOfSecondStart[fractionOfSecondLen])) {
            fractionOfSecondLen++;

            secondValue = secondValue * 10;
        }

        if (fractionOfSecondLen > 0) {
            int fractionOfSecond = StringUtils_digitsToInt(fractionOfSecondStart, fractionOfSecondLen);
            msOffset = (fractionOfSecond * 1000) / secondValue;
        }

        parsePos += fractionOfSecondLen;
    }

    t = 0;

    switch (*parsePos) {
    case 0: /* treat time as localtime */
        t = mktime(&tmTime);
        break;
    case 'Z': /* treat time as GMT(UTC) time */
        t = timegm(&tmTime);
        break;
    case '+': /* subtract offset */
        {
            int secondsOffset = getSecondsOffset(parsePos + 1);
			t = timegm(&tmTime);
            t = t - secondsOffset;
        }
        break;
    case '-': /* add offset */
        {
            int secondsOffset = getSecondsOffset(parsePos + 1);
			t = timegm(&tmTime);
            t = t + secondsOffset;
        }
        break;
    default:
        return -1;
    }

    msTime = (uint64_t) t * 1000L;

    msTime += msOffset;

    return msTime;
}

void
memcpyReverseByteOrder(uint8_t* dst, const uint8_t* src, int size)
{
    int i = 0;
    for (i = 0; i < size; i++) {
        dst[i] = src[size - i - 1];
    }
}




