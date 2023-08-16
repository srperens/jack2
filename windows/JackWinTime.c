/*
 Copyright (C) 2004-2008 Grame

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackTime.h"
#include "JackError.h"

jack_time_t (*_jack_get_microseconds)(void) = 0;

static LARGE_INTEGER _jack_freq;
static UINT gPeriod = 0;

SERVER_EXPORT void JackSleep(long usec)
{
	Sleep(usec / 1000);
}

SERVER_EXPORT void InitTime()
{
    TIMECAPS caps;

    QueryPerformanceFrequency(&_jack_freq);
    if (timeGetDevCaps(&caps, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        jack_error("InitTime : could not get timer device");
    } else {
        gPeriod = caps.wPeriodMin;
        if (timeBeginPeriod(gPeriod) != TIMERR_NOERROR) {
            jack_error("InitTime : could not set minimum timer");
            gPeriod = 0;
        } else {
            jack_log("InitTime : multimedia timer resolution set to %d milliseconds", gPeriod);
       }
    }
}

SERVER_EXPORT void EndTime()
{
    if (gPeriod > 0) {
        timeEndPeriod(gPeriod);
    }
}

static jack_time_t jack_get_microseconds_from_qpc (void) 
{
	LARGE_INTEGER t1;
	QueryPerformanceCounter(&t1);
	return (jack_time_t)(((double)t1.QuadPart) / ((double)_jack_freq.QuadPart) * 1000000.0);
}

static jack_time_t jack_get_microseconds_from_system (void)
{
    FILETIME ft;
    //GetSystemTimeAsFileTime(&ft);   //windows2000  -4
    GetSystemTimePreciseAsFileTime(&ft);  //windows8

    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    unsigned long long valueAsHns = li.QuadPart;
    unsigned long long valueAsUs = valueAsHns/10;

    return (jack_time_t) valueAsUs;
}

void SetClockSource(jack_timer_type_t source)
{
    jack_log("Clock source : %s", ClockSourceName(source));

	switch (source)
	{
        case JACK_TIMER_QPC:
            _jack_get_microseconds = jack_get_microseconds_from_qpc;
            break;
        case JACK_TIMER_SYSTEM_CLOCK:
            default:
            _jack_get_microseconds = jack_get_microseconds_from_system;
            break;
	}
}

const char* ClockSourceName(jack_timer_type_t source)
{
    switch (source) {
        case JACK_TIMER_QPC:
            return "QueryPerformanceCounter";
        case JACK_TIMER_SYSTEM_CLOCK:
            return "system clock via GetSystemTimeAsFileTime";
	}

	/* what is wrong with gcc ? */
	return "unknown";
}

SERVER_EXPORT jack_time_t GetMicroSeconds()
{
	return _jack_get_microseconds();
}
