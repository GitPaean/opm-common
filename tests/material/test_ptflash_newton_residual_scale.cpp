// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA.

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
 * \brief The flash Newton residual used to contain the raw fugacity
 *        differences, which are of the order of the pressure.  Against the
 *        absolute tolerance of 1e-8 that made convergence at reservoir
 *        pressure a race with round-off: at 55 bar the evaluation noise
 *        floor of f_liquid - f_vapor is around 1e-8 Pa, so Newton could
 *        enter at a residual it can never reduce and grind through its full
 *        iteration budget on a state it had already solved.  A CO2 injection
 *        run on a 60x60x20 deck recorded 15000+ such stalls in its first
 *        simulated day, every one entering at a residual of 1.0e-8 to
 *        2.4e-8 — the floor tracking the field pressure — and every one
 *        thrown away after 1000 iterations.  Scaling the fugacity rows by
 *        the pressure makes the criterion dimensionless without changing
 *        the iterates, and Newton accepts these states immediately.
 *
 *        The state below is one of the recorded stalls, verbatim: the
 *        deck's component properties and the entry (z, p, T, K, L) the
 *        solver logged when it gave up.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlashNewtonResidualScale
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <cmath>

namespace {

using Scalar = double;
constexpr int numComponents = 3;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using Evaluation = Opm::DenseAd::Evaluation<Scalar, numComponents + 1>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;
using PtFlash = Opm::PTFlash<Scalar, FluidSystem, true>;
using EOSType = Opm::CompositionalConfig::EOSType;

// the CO2/METHANE/DECANE description of the recording deck
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        FluidSystem::addComponent(CompParam{"CO2", 44.0, 304.128, 73.773e5, 0.09412, 0.22394});
        FluidSystem::addComponent(CompParam{"C1", 16.04, 190.564, 45.992e5, 0.09863, 0.01142});
        FluidSystem::addComponent(CompParam{"C10", 142.28, 617.7, 21.03e5, 0.60980, 0.4884});
    }
};

// The recorded stall.  K and L are the values Newton entered with, i.e.
// after the five preprocessing substitution steps of "ssi+newton"; solving
// with the plain "newton" method from them replays the recorded composition
// update exactly.
FluidState makeRecordedState()
{
    FluidState fs;
    for (unsigned phaseIdx = 0; phaseIdx < 2; ++phaseIdx)
        fs.setPressure(phaseIdx, 5497102.804812587);
    fs.setTemperature(423.15);
    fs.setMoleFraction(0, 0.09999622229510055);
    fs.setMoleFraction(1, 0.7999645720978721);
    fs.setMoleFraction(2, 0.10003920560702739);
    fs.setKvalue(0, 2.8530036056955432);
    fs.setKvalue(1, 5.029874030171108);
    fs.setKvalue(2, 0.028548709684505142);
    fs.setLvalue(0.10105909849980388);
    return fs;
}

} // anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(NewtonAcceptsTheRecordedStall)
{
    auto fs = makeRecordedState();
    bool singlePhase = true;
    BOOST_REQUIRE_NO_THROW(singlePhase = PtFlash::solve(fs, "newton", 1e-8, EOSType::PR));
    BOOST_CHECK(!singlePhase);
    const Scalar L = Opm::getValue(fs.L());
    BOOST_CHECK(L > 0.05 && L < 0.15);
}

BOOST_AUTO_TEST_CASE(NewtonMatchesSsi)
{
    auto fsSsi = makeRecordedState();
    PtFlash::solve(fsSsi, "ssi", 1e-8, EOSType::PR);

    auto fsNewton = makeRecordedState();
    PtFlash::solve(fsNewton, "newton", 1e-8, EOSType::PR);

    BOOST_CHECK_SMALL(std::abs(Opm::getValue(fsNewton.L()) - Opm::getValue(fsSsi.L())), 1e-6);
    for (unsigned phaseIdx = 0; phaseIdx < 2; ++phaseIdx) {
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            BOOST_CHECK_SMALL(std::abs(
                Opm::getValue(fsNewton.moleFraction(phaseIdx, compIdx)) -
                Opm::getValue(fsSsi.moleFraction(phaseIdx, compIdx))), 1e-6);
        }
    }
}
