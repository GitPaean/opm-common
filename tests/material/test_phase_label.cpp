/*
  Copyright 2026, SINTEF Digital

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
/*!
 * \file
 *
 * \brief Tests the phase identification parameter that labels a
 *        single-phase compositional fluid liquid or vapour.
 *
 * Li's pseudo-critical temperature only sees the temperature, so it is
 * wrong on both sides: a dense fluid above its pseudo-critical temperature
 * is called vapour, and a lean gas below it is called liquid. The parameter
 * is evaluated at the actual pressure, and the cases below pin the two
 * failures it corrects along with the ideal-gas and compressed-liquid
 * limits.
 */
#include "config.h"

#define BOOST_TEST_MODULE PhaseLabel
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <dune/common/fvector.hh>

namespace {

using Scalar = double;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;
using Flash = Opm::PTFlash<Scalar, FluidSystem>;
using FluidState = Opm::CompositionalFluidState<Scalar, FluidSystem>;
using ComponentVector = Dune::FieldVector<Scalar, 3>;
using EOSType = Opm::CompositionalConfig::EOSType;

constexpr EOSType eosType = EOSType::PR;

template <class Component>
void registerComponent()
{
    using CompParam = typename FluidSystem::ComponentParam;
    FluidSystem::addComponent(CompParam{Component::name(), Component::molarMass(),
                                        Component::criticalTemperature(),
                                        Component::criticalPressure(),
                                        Component::criticalVolume(),
                                        Component::acentricFactor()});
}

// Components in the order CO2, C1, C10.
struct Fixture
{
    Fixture()
    {
        FluidSystem::init();
        registerComponent<Opm::SimpleCO2<Scalar>>();
        registerComponent<Opm::C1<Scalar>>();
        registerComponent<Opm::C10<Scalar>>();
    }
};

FluidState stateAt(Scalar temperature, Scalar pressure)
{
    FluidState fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    return fs;
}

Scalar pip(Scalar temperature, Scalar pressure, const ComponentVector& z)
{
    return Flash::phaseIdentificationParameter(stateAt(temperature, pressure), z, eosType);
}

Scalar liLabel(Scalar temperature, Scalar pressure, const ComponentVector& z)
{
    return Flash::li_single_phase_label_(stateAt(temperature, pressure), z, 0);
}

} // namespace

BOOST_GLOBAL_FIXTURE(Fixture);

// An ideal gas has a parameter of exactly one; a dilute real gas sits just
// below it.
BOOST_AUTO_TEST_CASE(DiluteGasSitsJustBelowOne)
{
    const ComponentVector z{0.0, 1.0, 0.0};
    const Scalar value = pip(300.0, 1.0e5, z);
    BOOST_TEST_MESSAGE("methane at 1 bar: " << value);
    BOOST_CHECK_LT(value, 1.0);
    BOOST_CHECK_GT(value, 0.99);
}

// Decane at room conditions is a compressed liquid. The cubic has three
// roots there, and the parameter must be taken on the stable one.
BOOST_AUTO_TEST_CASE(CompressedLiquidIsAboveOne)
{
    const ComponentVector z{0.0, 0.0, 1.0};
    const Scalar value = pip(300.0, 1.0e5, z);
    BOOST_TEST_MESSAGE("decane at 1 bar: " << value);
    BOOST_CHECK_GT(value, 1.0);
}

// A dense CO2-rich fluid above the CO2 critical temperature. Li's method
// sees T above its pseudo-critical temperature and calls it vapour; at
// 200 bar it is liquid-like.
BOOST_AUTO_TEST_CASE(DenseSupercriticalFluidIsLiquidLike)
{
    const ComponentVector z{0.95, 0.05, 0.0};
    const Scalar T = 320.0;
    const Scalar p = 200.0e5;
    const Scalar value = pip(T, p, z);
    BOOST_TEST_MESSAGE("CO2-rich at 320 K, 200 bar: " << value);
    BOOST_CHECK_EQUAL(liLabel(T, p, z), 0.0);
    BOOST_CHECK_GT(value, 1.0);
}

// A CO2/methane gas at low pressure. Li's pseudo-critical temperature lies
// above T, so it calls the fluid liquid; at 5 bar it is a gas.
BOOST_AUTO_TEST_CASE(LeanGasBelowPseudoCriticalTemperatureIsVapour)
{
    const ComponentVector z{0.6, 0.4, 0.0};
    const Scalar T = 250.0;
    const Scalar p = 5.0e5;
    const Scalar value = pip(T, p, z);
    BOOST_TEST_MESSAGE("CO2/C1 at 250 K, 5 bar: " << value);
    BOOST_CHECK_EQUAL(liLabel(T, p, z), 1.0);
    BOOST_CHECK_LT(value, 1.0);
}

// Along a supercritical isotherm the label changes exactly once, from
// vapour-like at low pressure to liquid-like once the fluid is dense.
BOOST_AUTO_TEST_CASE(SupercriticalIsothermCrossesOnce)
{
    const ComponentVector z{0.0, 1.0, 0.0}; // methane, Tc = 190.6 K
    const Scalar T = 300.0;
    int crossings = 0;
    bool wasLiquid = pip(T, 1.0e5, z) > 1.0;
    BOOST_CHECK(!wasLiquid);
    Scalar p = 1.0e5;
    for (; p <= 2000.0e5; p *= 1.05) {
        const bool liquid = pip(T, p, z) > 1.0;
        if (liquid != wasLiquid) {
            ++crossings;
            wasLiquid = liquid;
        }
    }
    BOOST_TEST_MESSAGE("methane at 300 K, " << p / 1.05e5 << " bar: " << pip(T, p / 1.05, z));
    BOOST_CHECK_EQUAL(crossings, 1);
    BOOST_CHECK(wasLiquid);
}
