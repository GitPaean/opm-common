/*
  Copyright 2026 SINTEF Digital, Mathematics & Cybernetics.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#define BOOST_TEST_MODULE Test TimeService
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <ctime>
#include <limits>

// TimeStampUTC(std::time_t) breaks a time_t into civil time itself rather than
// calling std::gmtime, so these check the range std::gmtime would not have
// covered: dates a C runtime may refuse, and instants before the epoch.

BOOST_AUTO_TEST_CASE(FromTimeT_Epoch)
{
    const auto ts = Opm::TimeStampUTC{ std::time_t{0} };

    BOOST_CHECK_EQUAL(ts.year(),    1970);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
    BOOST_CHECK_EQUAL(ts.minutes(),    0);
    BOOST_CHECK_EQUAL(ts.seconds(),    0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_BeforeEpoch)
{
    // One second before the epoch. Needs floor division of a negative time_t:
    // truncation towards zero would land on 1970-01-01.
    const auto ts = Opm::TimeStampUTC{ std::time_t{-1} };

    BOOST_CHECK_EQUAL(ts.year(),    1969);
    BOOST_CHECK_EQUAL(ts.month(),     12);
    BOOST_CHECK_EQUAL(ts.day(),       31);
    BOOST_CHECK_EQUAL(ts.hour(),      23);
    BOOST_CHECK_EQUAL(ts.minutes(),   59);
    BOOST_CHECK_EQUAL(ts.seconds(),   59);
}

BOOST_AUTO_TEST_CASE(FromTimeT_LeapDay)
{
    // 2000-02-29, the century leap year the 400-year rule keeps.
    const auto ts = Opm::TimeStampUTC{ std::time_t{951'782'400} };

    BOOST_CHECK_EQUAL(ts.year(),    2000);
    BOOST_CHECK_EQUAL(ts.month(),      2);
    BOOST_CHECK_EQUAL(ts.day(),       29);
}

BOOST_AUTO_TEST_CASE(FromTimeT_BeyondYear3000)
{
    // 3000-01-01T00:00:00Z. Some C runtimes refuse this, which is why the
    // conversion no longer goes through std::gmtime. Schedules do reach here.
    const auto ts = Opm::TimeStampUTC{ std::time_t{32'503'680'000} };

    BOOST_CHECK_EQUAL(ts.year(),    3000);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
}

BOOST_AUTO_TEST_CASE(RoundTripThroughTimeT)
{
    // asTimeT() and TimeStampUTC(time_t) use days_from_civil() and
    // civil_from_days() respectively; they must be exact inverses.
    for (const auto& ymd : { Opm::TimeStampUTC::YMD{1901,  1,  1},
                             Opm::TimeStampUTC::YMD{1969, 12, 31},
                             Opm::TimeStampUTC::YMD{1970,  1,  1},
                             Opm::TimeStampUTC::YMD{2000,  2, 29},
                             Opm::TimeStampUTC::YMD{2026,  8,  5},
                             Opm::TimeStampUTC::YMD{2100,  3,  1},
                             Opm::TimeStampUTC::YMD{3000,  1,  1} })
    {
        const auto stamp = Opm::TimeStampUTC{ ymd }.hour(13).minutes(37).seconds(7);
        const auto back  = Opm::TimeStampUTC{ Opm::asTimeT(stamp) };

        BOOST_CHECK_EQUAL(back.year(),    ymd.year);
        BOOST_CHECK_EQUAL(back.month(),   ymd.month);
        BOOST_CHECK_EQUAL(back.day(),     ymd.day);
        BOOST_CHECK_EQUAL(back.hour(),    13);
        BOOST_CHECK_EQUAL(back.minutes(), 37);
        BOOST_CHECK_EQUAL(back.seconds(),  7);
    }
}

// portable_gmtime() is the std::gmtime() replacement on the Eclipse output
// path, and that path reads more than the date: DoubHEAD takes tm_yday, and
// a drop-in replacement has to agree on tm_wday too. Dates with a known
// weekday and day of year, on both sides of the epoch and beyond year 3000.

BOOST_AUTO_TEST_CASE(PortableGmtime_DayOfYearAndWeekday)
{
    struct Case { std::time_t t; int year; int mon; int mday; int yday; int wday; };
    for (const auto& c : { Case{               0, 1970,  1,  1,   0, 4 },   // Thursday
                           Case{              -1, 1969, 12, 31, 364, 3 },   // Wednesday
                           Case{     951'782'400, 2000,  2, 29,  59, 2 },   // Tuesday
                           Case{     978'220'800, 2000, 12, 31, 365, 0 },   // Sunday, a leap year's last day
                           Case{  32'503'680'000, 3000,  1,  1,   0, 3 } }) // Wednesday
    {
        const auto tm = Opm::TimeService::portable_gmtime(c.t);

        BOOST_CHECK_EQUAL(tm.tm_year + 1900, c.year);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1,     c.mon);
        BOOST_CHECK_EQUAL(tm.tm_mday,        c.mday);
        BOOST_CHECK_EQUAL(tm.tm_yday,        c.yday);
        BOOST_CHECK_EQUAL(tm.tm_wday,        c.wday);
        BOOST_CHECK_EQUAL(tm.tm_isdst,       0);
    }
}

BOOST_AUTO_TEST_CASE(PortableGmtime_MinimumTimeT)
{
    // The bottom of the range. The floor division used to subtract 86399
    // from t before dividing, which overflows exactly here; the conversion
    // has to stay defined all the way down. The year is far outside int, so
    // only what does not depend on it is checked: the time of day is in
    // range and the same one day later, which is also the next weekday.
    constexpr auto t = std::numeric_limits<std::time_t>::min();
    const auto a = Opm::TimeService::portable_gmtime(t);
    const auto b = Opm::TimeService::portable_gmtime(t + 86400);

    BOOST_CHECK(0 <= a.tm_hour && a.tm_hour < 24);
    BOOST_CHECK(0 <= a.tm_min  && a.tm_min  < 60);
    BOOST_CHECK(0 <= a.tm_sec  && a.tm_sec  < 60);
    BOOST_CHECK(0 <= a.tm_wday && a.tm_wday < 7);
    BOOST_CHECK(0 <= a.tm_yday && a.tm_yday < 366);

    BOOST_CHECK_EQUAL(a.tm_hour, b.tm_hour);
    BOOST_CHECK_EQUAL(a.tm_min,  b.tm_min);
    BOOST_CHECK_EQUAL(a.tm_sec,  b.tm_sec);
    BOOST_CHECK_EQUAL((a.tm_wday + 1) % 7, b.tm_wday);
}
