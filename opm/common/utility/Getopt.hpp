/*
  Copyright 2026 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_GETOPT_HPP
#define OPM_GETOPT_HPP

// getopt() and getopt_long() for the command line programs.  They come from
// <getopt.h> where that header exists.  Otherwise the definitions below are
// used, which cover what the programs need: short options, grouped (-ab) or
// with an argument (-r 5, -r5); long options as --name, --name=value or
// --name value, and an unambiguous prefix of a long name; "--" to end the
// options.  Options are read up to the first argument that is not one.

#if __has_include(<getopt.h>)

#include <getopt.h>

#else

#include <cstdio>
#include <cstring>

inline char* optarg = nullptr;
inline int optind = 1;
inline int opterr = 1;
inline int optopt = 0;

namespace Opm::detail {

//! \brief Position within a group of short options (-ab); 1 between arguments.
inline int getoptPosition = 1;

} // namespace Opm::detail

inline int getopt(int argc, char* const argv[], const char* optstring)
{
    auto& pos = Opm::detail::getoptPosition;
    const bool report = (opterr != 0) && (*optstring != ':');

    optarg = nullptr;
    if (optind >= argc) {
        return -1;
    }

    const char* arg = argv[optind];
    if ((arg[0] != '-') || (arg[1] == '\0')) {
        return -1;
    }

    if ((arg[1] == '-') && (arg[2] == '\0')) {
        ++optind;
        return -1;
    }

    const int optchar = static_cast<unsigned char>(arg[pos]);
    const char* match = std::strchr(optstring, optchar);
    if ((optchar == ':') || (match == nullptr)) {
        optopt = optchar;
        if (report) {
            std::fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], optchar);
        }
        if (arg[++pos] == '\0') {
            ++optind;
            pos = 1;
        }
        return '?';
    }

    if (match[1] == ':') {
        if (arg[pos + 1] != '\0') {
            optarg = const_cast<char*>(&arg[pos + 1]);
            ++optind;
        }
        else if (optind + 1 < argc) {
            optarg = argv[optind + 1];
            optind += 2;
        }
        else {
            optopt = optchar;
            ++optind;
            pos = 1;
            if (report) {
                std::fprintf(stderr, "%s: option requires an argument -- '%c'\n",
                             argv[0], optchar);
            }
            return (*optstring == ':') ? ':' : '?';
        }
        pos = 1;
    }
    else if (arg[++pos] == '\0') {
        ++optind;
        pos = 1;
    }

    return optchar;
}

struct option
{
    const char* name;
    int has_arg;
    int* flag;
    int val;
};

inline constexpr int no_argument = 0;
inline constexpr int required_argument = 1;
inline constexpr int optional_argument = 2;

inline int getopt_long(int argc, char* const argv[], const char* optstring,
                       const option* longopts, int* longindex)
{
    if ((Opm::detail::getoptPosition != 1) || (optind >= argc)) {
        return getopt(argc, argv, optstring);
    }

    const char* name = argv[optind];
    if ((name[0] != '-') || (name[1] != '-') || (name[2] == '\0')) {
        return getopt(argc, argv, optstring);
    }

    name += 2;
    const bool report = (opterr != 0) && (*optstring != ':');
    const char* eq = std::strchr(name, '=');
    const std::size_t len = (eq != nullptr)
        ? static_cast<std::size_t>(eq - name)
        : std::strlen(name);

    int found = -1;
    bool ambiguous = false;
    for (int i = 0; longopts[i].name != nullptr; ++i) {
        if (std::strncmp(longopts[i].name, name, len) != 0) {
            continue;
        }
        if (std::strlen(longopts[i].name) == len) {
            found = i;
            ambiguous = false;
            break;
        }
        if (found < 0) {
            found = i;
        }
        else {
            ambiguous = true;
        }
    }

    optarg = nullptr;
    optopt = 0;
    ++optind;
    if ((found < 0) || ambiguous) {
        if (report) {
            std::fprintf(stderr, "%s: %s option '--%.*s'\n", argv[0],
                         ambiguous ? "ambiguous" : "unrecognized",
                         static_cast<int>(len), name);
        }
        return '?';
    }

    const option& opt = longopts[found];
    if (eq != nullptr) {
        if (opt.has_arg == no_argument) {
            optopt = opt.val;
            if (report) {
                std::fprintf(stderr, "%s: option '--%s' doesn't allow an argument\n",
                             argv[0], opt.name);
            }
            return '?';
        }
        optarg = const_cast<char*>(eq + 1);
    }
    else if (opt.has_arg == required_argument) {
        if (optind >= argc) {
            optopt = opt.val;
            if (report) {
                std::fprintf(stderr, "%s: option '--%s' requires an argument\n",
                             argv[0], opt.name);
            }
            return (*optstring == ':') ? ':' : '?';
        }
        optarg = argv[optind++];
    }

    if (longindex != nullptr) {
        *longindex = found;
    }

    if (opt.flag != nullptr) {
        *opt.flag = opt.val;
        return 0;
    }

    return opt.val;
}

#endif // __has_include(<getopt.h>)

#endif // OPM_GETOPT_HPP
