/*
  Copyright 2026 Equinor ASA.

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

#include <chrono>
#include <ctime>
#include <limits>
#include <stdexcept>

// TimeStampUTC(std::time_t) breaks a time_t into civil time with the <chrono>
// calendar types rather than calling std::gmtime(), so these check the range
// std::gmtime() would not have covered: dates a C runtime may refuse, and
// instants before the epoch.

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
    // One second before the epoch. Needs the floor division of a negative
    // count that hh_mm_ss and floor<days>() do: truncation towards zero
    // would land on 1970-01-01.
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
    // 3001-01-01T00:00:00Z: the first instant past MSVC's _gmtime64 range,
    // which ends with year 3000. That refusal is why the conversion no
    // longer goes through std::gmtime(). Schedules do reach here.
    const auto ts = Opm::TimeStampUTC{ std::time_t{32'535'216'000} };

    BOOST_CHECK_EQUAL(ts.year(),    3001);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_FarBeforeEpoch)
{
    // 0001-01-01T00:00:00Z, 62 billion seconds before the epoch: the
    // negative floor division well away from anything a C runtime handles.
    const auto ts = Opm::TimeStampUTC{ std::time_t{-62'135'596'800} };

    BOOST_CHECK_EQUAL(ts.year(),       1);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
    BOOST_CHECK_EQUAL(ts.minutes(),    0);
    BOOST_CHECK_EQUAL(ts.seconds(),    0);
}

BOOST_AUTO_TEST_CASE(RoundTripThroughTimeT)
{
    // asTimeT() and TimeStampUTC(time_t) are the two directions; they must
    // be exact inverses, including where a 32-bit day count would have
    // overflowed (anything outside roughly 1902-2038).
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

BOOST_AUTO_TEST_CASE(AssignFromTimeT)
{
    // operator=(time_t) is the same conversion as the constructor.
    auto ts = Opm::TimeStampUTC{};
    ts = std::time_t{32'535'216'000};

    BOOST_CHECK(ts == Opm::TimeStampUTC{ std::time_t{32'535'216'000} });
    BOOST_CHECK_EQUAL(ts.year(), 3001);
}

// mkdatetime() rejects an impossible date by converting it and converting it
// back, and that check only works because the conversion lets a day beyond
// the month carry into the next one rather than normalising it away or
// refusing it. year_month_day counts on from the first of the month for a day
// the month does not hold, which is exactly that carry -- but it is a
// property worth pinning, since nothing else in the suite covers it.

BOOST_AUTO_TEST_CASE(MkDate_RejectsImpossibleDates)
{
    // 30 February 1983 counts on to 2 March, 33 January 2026 to 2 February,
    // and month 13 of 2026 to January 2027: in each case mkdate() is handed
    // back a date it did not ask for, and says so.
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1983,  2, 30), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026,  1, 33), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026, 13,  1), std::invalid_argument);

    // 29 February is not wrap-around in a leap year, and is in every other.
    BOOST_CHECK_NO_THROW(Opm::TimeService::mkdate(2000, 2, 29));
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1900, 2, 29), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026, 2, 29), std::invalid_argument);

    // The carry itself, which asTimeT() applies without the rejection.
    const auto wrapped = Opm::TimeStampUTC {
        Opm::asTimeT(Opm::TimeStampUTC{ Opm::TimeStampUTC::YMD{1983, 2, 30} })
    };

    BOOST_CHECK_EQUAL(wrapped.year(),  1983);
    BOOST_CHECK_EQUAL(wrapped.month(),    3);
    BOOST_CHECK_EQUAL(wrapped.day(),      2);
}

BOOST_AUTO_TEST_CASE(EndsOfTheCalendar)
{
    // The last and the first instant std::chrono::year can represent:
    // 32767-12-31T23:59:59Z and -32767-01-01T00:00:00Z. Both convert, and
    // what one direction hands back the other takes back.
    {
        const auto t  = std::time_t{971'890'963'199};
        const auto ts = Opm::TimeStampUTC{ t };

        BOOST_CHECK_EQUAL(ts.year(),    32767);
        BOOST_CHECK_EQUAL(ts.month(),      12);
        BOOST_CHECK_EQUAL(ts.day(),        31);
        BOOST_CHECK_EQUAL(ts.hour(),       23);
        BOOST_CHECK_EQUAL(ts.minutes(),    59);
        BOOST_CHECK_EQUAL(ts.seconds(),    59);
        BOOST_CHECK_EQUAL(Opm::asTimeT(ts), t);
    }

    {
        // Taken from <chrono> itself, and checked against the arithmetic:
        // 12'687'428 days before the epoch.
        namespace ch = std::chrono;
        const auto first_day = ch::sys_days{ch::year::min() / ch::January / 1}
            .time_since_epoch().count();
        const auto t = std::time_t{first_day} * 86400;
        BOOST_CHECK_EQUAL(t, std::time_t{-1'096'193'779'200});

        const auto ts = Opm::TimeStampUTC{ t };

        BOOST_CHECK_EQUAL(ts.year(),   -32767);
        BOOST_CHECK_EQUAL(ts.month(),       1);
        BOOST_CHECK_EQUAL(ts.day(),         1);
        BOOST_CHECK_EQUAL(Opm::asTimeT(ts), t);
    }
}

BOOST_AUTO_TEST_CASE(OutsideTheCalendarIsRefused)
{
    // One second past either end, and the ends of the 64-bit range. A
    // refusal is the defined behaviour here, in place of a crash on the
    // platforms whose std::gmtime() returned nullptr, or a silently wrong
    // date on the ones whose day count overflowed.
    BOOST_CHECK_THROW(Opm::TimeStampUTC{ std::time_t{971'890'963'199} + 1 },
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC{ std::time_t{-1'096'193'779'200} - 1 },
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC{ std::numeric_limits<std::time_t>::max() },
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC{ std::numeric_limits<std::time_t>::min() },
                      std::out_of_range);

    // And the other direction: a year past either end, and a year inside the
    // calendar whose day of the month or time of day carries the instant
    // past it. The last valid instants beside them convert.
    const auto asT = [](const int y, const int mo, const int d,
                        const int h, const int mi, const int s)
    {
        return Opm::asTimeT(Opm::TimeStampUTC{ Opm::TimeStampUTC::YMD{y, mo, d} }
                            .hour(h).minutes(mi).seconds(s));
    };

    BOOST_CHECK_EQUAL(asT(32767, 12, 31, 23, 59, 59), std::time_t{971'890'963'199});
    BOOST_CHECK_THROW(asT(32768,  1,  1,  0,  0,  0), std::out_of_range);
    BOOST_CHECK_THROW(asT(32767, 12, 32,  0,  0,  0), std::out_of_range);
    BOOST_CHECK_THROW(asT(32767, 12, 31, 24,  0,  0), std::out_of_range);

    BOOST_CHECK_EQUAL(asT(-32767, 1, 1, 0, 0, 0), std::time_t{-1'096'193'779'200});
    BOOST_CHECK_THROW(asT(-32768, 1, 1, 0,  0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(-32767, 1, 1, 0,  0, -1), std::out_of_range);

    // An absurd field ends in that refusal rather than in an overflow on the
    // way there: the arithmetic is done in long long, not in a std::tm's int.
    const auto intMax = std::numeric_limits<int>::max();
    const auto intMin = std::numeric_limits<int>::min();

    BOOST_CHECK_THROW(asT(intMax, 1, 1, 0, 0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(intMin, 1, 1, 0, 0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(2026, intMax, 1, 0, 0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(2026, intMin, 1, 0, 0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(2026, 1, intMax, 0, 0, 0), std::out_of_range);
    BOOST_CHECK_THROW(asT(2026, 1, 1, intMax, 0, 0), std::out_of_range);
}
