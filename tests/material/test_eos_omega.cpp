// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \brief OMEGAA and OMEGAB replace the constants of the cubic equation of
 *        state per component, and their defaults leave it unchanged.
 */
#include "config.h"

#define BOOST_TEST_MODULE EosOmega
#include <boost/test/unit_test.hpp>

#include <opm/material/eos/CubicEOSParams.hpp>
#include <opm/material/eos/PRParams.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <array>
#include <string>

namespace
{

using Scalar = double;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;
using EosParams = Opm::CubicEOSParams<Scalar, FluidSystem, FluidSystem::oilPhaseIdx>;
using PR = Opm::PRParams<Scalar, FluidSystem>;

constexpr Scalar pressure = 150e5;
constexpr Scalar temperature = 380.0;

// Initializes the fluid system from a three-component Peng-Robinson deck
// with the given OMEGAA and OMEGAB input, and returns its EoS parameters.
EosParams
parametersFromDeck(const std::string& omegaKeywords)
{
    const auto deck = Opm::Parser {}.parseString(R"(
RUNSPEC
METRIC
DIMENS
 1 1 1 /
COMPS
3 /
TABDIMS
 1 /
OIL
GAS
WATER
GRID
DXV
 1 /
DYV
 1 /
DZV
 1 /
DEPTHZ
 4*2000 /
PROPS
CNAMES
 C1 C10 CO2 /
TCRIT
 190.6 617.7 304.2 /
PCRIT
 46.0 21.1 73.8 /
VCRIT
 0.0990 0.6240 0.0940 /
MW
 16.043 142.285 44.010 /
ACF
 0.008 0.4885 0.225 /
)" + omegaKeywords + R"(
SOLUTION
SCHEDULE
END
)");
    const auto eclState = Opm::EclipseState {deck};
    const auto schedule = Opm::Schedule {deck, eclState};
    FluidSystem::initFromState(eclState, schedule);

    EosParams params;
    params.setEOSType(Opm::CompositionalConfig::EOSType::PR);
    params.updatePure(temperature, pressure);
    return params;
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(DeckOmegasReplaceTheEquationConstants)
{
    const std::array<Scalar, 3> omegaA {0.44, 0.46, 0.45724};
    const std::array<Scalar, 3> omegaB {0.075, 0.08, 0.0778};
    const auto params = parametersFromDeck("OMEGAA\n 0.44 0.46 0.45724 /\n"
                                           "OMEGAB\n 0.075 0.08 0.0778 /\n");
    for (unsigned compIdx = 0; compIdx < 3; ++compIdx) {
        BOOST_TEST_CONTEXT("component " << compIdx)
        {
            BOOST_CHECK_EQUAL(FluidSystem::omegaA(compIdx).value(), omegaA[compIdx]);
            BOOST_CHECK_EQUAL(FluidSystem::omegaB(compIdx).value(), omegaB[compIdx]);

            const Scalar pr = pressure / FluidSystem::criticalPressure(compIdx);
            const Scalar Tr = temperature / FluidSystem::criticalTemperature(compIdx);
            const Scalar alpha = PR::calcOmegaA(temperature, compIdx, false) / PR::omegaA;
            BOOST_CHECK_CLOSE(params.Ai(compIdx), omegaA[compIdx] * alpha * pr / (Tr * Tr), 1e-12);
            BOOST_CHECK_CLOSE(params.Bi(compIdx), omegaB[compIdx] * pr / Tr, 1e-12);
        }
    }
}

// Without the keywords the deck carries the constants of the equation, so
// the parameters stay exactly what they were before OMEGAA and OMEGAB took
// effect.
BOOST_AUTO_TEST_CASE(DefaultOmegasKeepTheEquationConstants)
{
    const auto params = parametersFromDeck("");
    for (unsigned compIdx = 0; compIdx < 3; ++compIdx) {
        BOOST_TEST_CONTEXT("component " << compIdx)
        {
            const Scalar pr = pressure / FluidSystem::criticalPressure(compIdx);
            const Scalar Tr = temperature / FluidSystem::criticalTemperature(compIdx);
            BOOST_CHECK_EQUAL(params.Ai(compIdx),
                              PR::calcOmegaA(temperature, compIdx, false) * pr / (Tr * Tr));
            BOOST_CHECK_EQUAL(params.Bi(compIdx), PR::calcOmegaB() * pr / Tr);
        }
    }
}
