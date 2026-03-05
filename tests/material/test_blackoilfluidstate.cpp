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

#include <string_view>
#include <type_traits>
#include <vector>

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

// Verify that createFluidState is a static method with the expected return type.
// This is a compile-time check; no fluid system initialization is required.
BOOST_AUTO_TEST_CASE_TEMPLATE(CreateFluidStateSignature, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 2>;

    // The return type of createFluidState<Evaluation> should be
    // BlackOilFluidState<Evaluation, FluidSystem, ...> with the same
    // boolean template args as the calling specialization.
    using FluidStateType = Opm::BlackOilFluidState<Scalar, FluidSystem,
                                                    /*storeTemperature=*/false,
                                                    /*storeEnthalpy=*/false,
                                                    /*enableDissolution=*/true>;

    using ExpectedReturn = Opm::BlackOilFluidState<Evaluation, FluidSystem,
                                                    /*storeTemperature=*/false,
                                                    /*storeEnthalpy=*/false,
                                                    /*enableDissolution=*/true>;

    // Verify that the return type of createFluidState<Evaluation> matches.
    // We use decltype on a pointer-to-member-function to avoid calling the function.
    using FnPtr = ExpectedReturn (*)(const std::vector<Evaluation>&,
                                     const Evaluation&,
                                     const Evaluation&,
                                     Scalar,
                                     int,
                                     std::string_view);

    static_assert(
        std::is_same_v<FnPtr, decltype(&FluidStateType::template createFluidState<Evaluation>)>,
        "createFluidState return type or signature does not match expectations");

    BOOST_TEST(true); // Ensure the test is actually executed
}
