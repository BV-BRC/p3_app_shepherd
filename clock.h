#ifndef _clock_h
#define _clock_h

#include <chrono>
#include <sys/time.h>

using p3_clock = std::chrono::system_clock;
using p3_time_point = p3_clock::time_point;
using p3_duration = p3_clock::duration;

inline timeval to_timeval(p3_time_point tp)
{
    using namespace std::chrono;
    auto s = time_point_cast<seconds>(tp);
    if (s > tp)
	s = s - seconds{1};
    auto us = duration_cast<microseconds>(tp - s);
    timeval tv;
    tv.tv_sec = s.time_since_epoch().count();
    tv.tv_usec = us.count();
    return tv;
}

inline p3_time_point to_time_point(timeval tv)
{
    using namespace std::chrono;
    return p3_time_point{seconds{tv.tv_sec} + microseconds{tv.tv_usec}};
}

#endif
