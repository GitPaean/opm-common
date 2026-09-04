/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#include <opm/common/utility/TimeService.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <chrono>
#include <ctime>
#include <limits>
#include <utility>
#include <stdexcept>
#include <string>

namespace {

    // The calendar these conversions work in. std::chrono::year runs from
    // -32767 to 32767; no schedule is anywhere near either end, and an
    // instant outside it is refused rather than silently mis-dated.
    constexpr auto first_day =
        std::chrono::sys_days{std::chrono::year::min() / std::chrono::January / 1};
    constexpr auto last_day =
        std::chrono::sys_days{std::chrono::year::max() / std::chrono::December / 31};

    // A civil date and time of day as UTC seconds since the epoch.
    //
    // The month is carried into the year, and a day outside the month counts
    // on from its first, because std::chrono::year_month_day converts that
    // way: 33 January is 2 February. mkdatetime() depends on the carry to
    // spot a date that does not exist, so do not normalise it away.
    std::chrono::sys_seconds
    civil_to_sys_seconds(const long long year,
                         const long long month,   // 1-based
                         const long long day,
                         const long long hour,
                         const long long minute,
                         const long long second)
    {
        namespace ch = std::chrono;

        // Carry the month first, in long long, so that an absurd value ends
        // in the refusal below rather than in an overflow on the way there.
        auto yr = year;
        auto mo = month - 1;             // 0-based for the arithmetic
        if (mo > 11) {
            yr += mo / 12;
            mo %= 12;
        }
        else if (mo < 0) {
            const auto years_diff = (11 - mo) / 12;
            yr -= years_diff;
            mo += 12 * years_diff;
        }

        if ((yr < static_cast<int>(ch::year::min())) ||
            (yr > static_cast<int>(ch::year::max())))
        {
            throw std::out_of_range {
                "Calendar year " + std::to_string(yr) +
                " is outside the range std::chrono::year can represent"
            };
        }

        const auto first_of_month = ch::sys_days {
            ch::year{static_cast<int>(yr)} / ch::month{static_cast<unsigned>(mo + 1)} / 1
        };

        const auto t = ch::sys_seconds{first_of_month} +
            ch::days{day - 1} + ch::hours{hour} + ch::minutes{minute} + ch::seconds{second};

        // The day of the month or the time of day may have carried the
        // instant past the end of the calendar even though the year was
        // inside it - 32767-12-32, or 24:00 on the last day - and reading it
        // back would refuse it. Refuse it here instead.
        if ((t < ch::sys_seconds{first_day}) ||
            (t >= ch::sys_seconds{last_day} + ch::days{1}))
        {
            throw std::out_of_range {
                "Date " + std::to_string(yr) + "-" + std::to_string(mo + 1) + "-" +
                std::to_string(day) + " with the time of day added lies outside " +
                "the range std::chrono::year can represent"
            };
        }

        return t;
    }

    /*
       Break a time_t into UTC civil time with the <chrono> calendar types.

       This is where std::gmtime() used to be called, and dereferenced without
       a check. std::gmtime() returns nullptr for time points its C runtime
       cannot represent -- MSVC's refuses everything before 1970 and after
       year 3000, both of which simulation schedules legitimately reach -- so
       writing the first report step of such a schedule crashed. It also hands
       back a pointer to a static buffer, so two threads converting timestamps
       concurrently overwrite each other's result. year_month_day has neither
       problem, and is the exact inverse of the conversion in the other
       direction.
    */
    Opm::TimeStampUTC breakDownUTC(const std::time_t tp)
    {
        namespace ch = std::chrono;

        const auto t    = ch::sys_seconds{ch::seconds{tp}};
        const auto days = ch::floor<ch::days>(t);

        if ((days < first_day) || (days > last_day)) {
            throw std::out_of_range {
                "Time point " + std::to_string(static_cast<long long>(tp)) +
                " is outside the range of years std::chrono::year can represent"
            };
        }

        const auto ymd = ch::year_month_day{days};
        const auto hms = ch::hh_mm_ss{t - days};

        return Opm::TimeStampUTC {
            Opm::TimeStampUTC::YMD {
                static_cast<int>(ymd.year()),
                static_cast<int>(static_cast<unsigned>(ymd.month())),
                static_cast<int>(static_cast<unsigned>(ymd.day()))
            }
        }
        .hour(static_cast<int>(hms.hours().count()))
        .minutes(static_cast<int>(hms.minutes().count()))
        .seconds(static_cast<int>(hms.seconds().count()));
    }

} // anonymous namespace

namespace Opm {
namespace TimeService {

namespace {
    const std::unordered_map<std::string, int> month_indices = {
        {"JAN", 1},
        {"FEB", 2},
        {"MAR", 3},
        {"APR", 4},
        {"MAI", 5},
        {"MAY", 5},
        {"JUN", 6},
        {"JUL", 7},
        {"JLY", 7},
        {"AUG", 8},
        {"SEP", 9},
        {"OCT", 10},
        {"OKT", 10},
        {"NOV", 11},
        {"DEC", 12},
        {"DES", 12}};

    const std::unordered_map<int, std::string> month_names = {
        {1, "JAN"},
        {2, "FEB"},
        {3, "MAR"},
        {4, "APR"},
        {5, "MAY"},
        {6, "JUN"},
        {7, "JUL"},
        {8, "AUG"},
        {9, "SEP"},
        {10, "OCT"},
        {11, "NOV"},
        {12, "DEC"}};



} // anonymous namespace



const time_t system_clock_epoch = std::chrono::system_clock::to_time_t({});

time_point from_time_t(std::time_t t) {
    auto diff = std::difftime(t, system_clock_epoch);
    return time_point(std::chrono::seconds(static_cast<std::chrono::seconds::rep>(diff)));
}

std::time_t to_time_t(const time_point& tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count() + system_clock_epoch;
}


time_point now() {
    time_point epoch;
    auto default_now = std::chrono::system_clock::now();
    return epoch + std::chrono::duration_cast<Opm::time_point::duration>(default_now.time_since_epoch());
}

std::time_t advance(const std::time_t tp, const double sec)
{
    const auto t = Opm::TimeService::from_time_t(tp) + std::chrono::duration_cast<Opm::time_point::duration>(std::chrono::duration<double>(sec));
    return Opm::TimeService::to_time_t(t);
}

const std::unordered_map<std::string , int>& eclipseMonthIndices() {
    return month_indices;
}

int eclipseMonth(const std::string& name) {
    auto iter = month_indices.find(name);
    if (iter != month_indices.end())
        return iter->second;

    return std::stod(name);
}


const std::unordered_map<int, std::string>& eclipseMonthNames() {
    return month_names;
}

bool valid_month(const std::string& month_name)
{
    return month_indices.contains(month_name);
}

std::time_t mkdatetime(int in_year, int in_month, int in_day, int hour, int minute, int second) {
    const auto tp = TimeStampUTC{ TimeStampUTC::YMD { in_year, in_month, in_day } }
        .hour(hour).minutes(minute).seconds(second);

    std::time_t t = asTimeT(tp);
    {
        /*
          The underlying mktime( ) function will happily wrap
          around dates like January 33, this function will check
          that no such wrap-around has taken place.
        */
        const auto check = TimeStampUTC{ t };
        if ((in_day != check.day()) || (in_month != check.month()) || (in_year != check.year()))
            throw std::invalid_argument("Invalid input arguments for date.");
    }
    return t;
}

std::time_t mkdate(int in_year, int in_month, int in_day) {
    return mkdatetime(in_year , in_month , in_day, 0,0,0);
}

std::time_t timeFromEclipse(const DeckRecord &dateRecord) {
    const auto &dayItem = dateRecord.getItem(0);
    const auto &monthItem = dateRecord.getItem(1);
    const auto &yearItem = dateRecord.getItem(2);
    const auto &timeItem = dateRecord.getItem(3);

    int hour = 0, min = 0, second = 0;
    if (timeItem.hasValue(0)) {
        if (sscanf(timeItem.get<std::string>(0).c_str(), "%d:%d:%d" , &hour,&min,&second) != 3) {
            hour = min = second = 0;
        }
    }

    // Accept lower- and mixed-case month names.
    std::string monthname = uppercase(monthItem.get<std::string>(0));

    std::time_t date = mkdatetime(yearItem.get<int>(0),
                                  TimeService::eclipseMonthIndices().at(monthname),
                                  dayItem.get<int>(0),
                                  hour,
                                  min,
                                  second);
    return date;
}

}
}

namespace {





}

Opm::TimeStampUTC::TimeStampUTC(const std::time_t tp)
    : TimeStampUTC(breakDownUTC(tp))
{}

Opm::TimeStampUTC::TimeStampUTC(const Opm::TimeStampUTC::YMD& ymd,
                                int hour, int minutes, int seconds, int usec)
    : ymd_(ymd)
    , hour_(hour)
    , minutes_(minutes)
    , seconds_(seconds)
    , usec_(usec)
{}

Opm::TimeStampUTC& Opm::TimeStampUTC::operator=(const std::time_t tp)
{
    return *this = breakDownUTC(tp);
}

bool Opm::TimeStampUTC::operator==(const TimeStampUTC& data) const
{
    return ymd_ == data.ymd_ &&
           hour_ == data.hour_ &&
           minutes_ == data.minutes_ &&
           seconds_ == data.seconds_ &&
           usec_ == data.usec_;
}

Opm::TimeStampUTC::TimeStampUTC(const YMD& ymd)
    : ymd_{ std::move(ymd) }
{}

Opm::TimeStampUTC::TimeStampUTC(int year, int month, int day)
    : ymd_{ year, month, day }
{}

Opm::TimeStampUTC& Opm::TimeStampUTC::hour(const int h)
{
    this->hour_ = h;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::minutes(const int m)
{
    this->minutes_ = m;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::seconds(const int s)
{
    this->seconds_ = s;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::microseconds(const int us)
{
    this->usec_ = us;
    return *this;
}


std::tm Opm::asTm(const TimeStampUTC& tp)
{
    auto tm = std::tm{};

    tm.tm_year = tp.year()  - 1900;
    tm.tm_mon  = tp.month() -    1;
    tm.tm_mday = tp.day();
    tm.tm_hour = tp.hour();
    tm.tm_min  = tp.minutes();
    tm.tm_sec  = tp.seconds();

    return tm;
}

std::time_t Opm::asTimeT(const TimeStampUTC& tp)
{
    return civil_to_sys_seconds(tp.year(), tp.month(), tp.day(),
                                tp.hour(), tp.minutes(), tp.seconds())
        .time_since_epoch().count();
}

std::time_t Opm::asLocalTimeT(const TimeStampUTC& tp)
{
    // std::mktime() is the only way to apply the local time zone, and it
    // takes a std::tm.
    auto tm = asTm(tp);
    tm.tm_isdst = -1;

    return std::mktime(&tm);
}

Opm::TimeStampUTC Opm::operator+(const Opm::TimeStampUTC& lhs, std::chrono::duration<double> delta) {
    return Opm::TimeStampUTC( Opm::TimeService::advance(Opm::asTimeT(lhs) , delta.count()) );
}

Opm::time_point Opm::asTimePoint(const TimeStampUTC& ts)
{
    return Opm::TimeService::from_time_t( Opm::asTimeT(ts) );
}
