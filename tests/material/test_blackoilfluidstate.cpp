// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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
 * \brief This test ensures that the API of the black-oil fluid state conforms to the
 *        fluid state specification
 */
#include "config.h"

#include <boost/mpl/list.hpp>

#define BOOST_TEST_MODULE BlackOilFluidState
#include <boost/test/unit_test.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>
#include <opm/material/fluidstates/BlackOilFluidState.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>
#include <opm/material/checkFluidSystem.hpp>

using Types = boost::mpl::list<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(ApiConformance, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 2>;
    using FluidStateScalar = Opm::BlackOilFluidState<Scalar, FluidSystem>;
    using FluidState = Opm::BlackOilFluidState<Evaluation, FluidSystem>;

    FluidStateScalar fss{};
    checkFluidState<Scalar>(fss);
    FluidState fs{};
    checkFluidState<Evaluation>(fs);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SolventDisabled, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    // enableSolvent = false (default)
    using FluidState = Opm::BlackOilFluidState<Scalar, FluidSystem>;

    FluidState fs{};
    // When solvent is disabled, getters should return 0
    BOOST_CHECK_EQUAL(fs.solventSaturation(), Scalar(0.0));
    BOOST_CHECK_EQUAL(fs.rsSolw(), Scalar(0.0));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SolventEnabled, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    // Template params: ScalarT, FluidSystemT, storeTemperature, storeEnthalpy,
    //   enableDissolution, enableVapwat, enableBrine, enableSaltPrecipitation,
    //   enableDissolutionInWater, enableSolvent
    using FluidState = Opm::BlackOilFluidState<Scalar, FluidSystem,
                                               false, false, true, false,
                                               false, false, false, true>;

    FluidState fs{};

    // Test setter and getter for solvent saturation
    fs.setSolventSaturation(Scalar(0.15));
    BOOST_CHECK_EQUAL(fs.solventSaturation(), Scalar(0.15));

    // Test setter and getter for rsSolw
    fs.setRsSolw(Scalar(0.05));
    BOOST_CHECK_EQUAL(fs.rsSolw(), Scalar(0.05));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SolventAssign, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    using FluidState = Opm::BlackOilFluidState<Scalar, FluidSystem,
                                               false, false, true, false,
                                               false, false, false, true>;

    FluidState src{};
    src.setPvtRegionIndex(0);
    src.setSolventSaturation(Scalar(0.25));
    src.setRsSolw(Scalar(0.10));

    // Set required phase quantities
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
        src.setSaturation(phaseIdx, Scalar(1.0 / FluidSystem::numPhases));
        src.setPressure(phaseIdx, Scalar(1e5));
        src.setDensity(phaseIdx, Scalar(1000.0));
        src.setInvB(phaseIdx, Scalar(1.0));
    }
    src.setRs(Scalar(0.0));
    src.setRv(Scalar(0.0));

    // Assign from source to destination
    FluidState dst{};
    dst.assign(src);

    BOOST_CHECK_EQUAL(dst.solventSaturation(), Scalar(0.25));
    BOOST_CHECK_EQUAL(dst.rsSolw(), Scalar(0.10));
}
