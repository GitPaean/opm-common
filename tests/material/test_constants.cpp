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

#define BOOST_TEST_MODULE Test Constants

#include <boost/test/unit_test.hpp>

#include <opm/material/Constants.hpp>
#include <opm/material/components/iapws/Common.hpp>
#include <opm/material/densead/Evaluation.hpp>

#include <numbers>
#include <type_traits>

// The derived constants - kb and hRed in Constants, Rs and
// criticalMolarVolume in IAPWS::Common - are constant expressions computed
// in the floating-point type behind Scalar. They must equal the quotient
// written directly in that type, bit for bit: for float and double, and
// for an autodiff Evaluation on either, whose constants must carry the
// value of the plain type and no derivatives. hRed has no use in the
// repository, so this is also what instantiates it.

namespace {

template <class Scalar>
void checkDerivedConstants()
{
    using C = Opm::Constants<Scalar>;
    using W = Opm::IAPWS::Common<Scalar>;

    BOOST_CHECK_EQUAL(C::kb,   Scalar(8.314472) / Scalar(6.02214179e23));
    BOOST_CHECK_EQUAL(C::hRed, Scalar(6.62606896e-34) / (Scalar(2) * std::numbers::pi_v<Scalar>));
    BOOST_CHECK_EQUAL(W::Rs,   Scalar(8.314472) / Scalar(18.01518e-3));
    BOOST_CHECK_EQUAL(W::criticalMolarVolume, Scalar(18.01518e-3) / Scalar(322.0));
}

template <class Eval>
void checkAutodiffConstants()
{
    using Value = typename Eval::ValueType;
    static_assert(std::is_same_v<typename Opm::detail::ConstantsComputeType<Eval>::type, Value>,
                  "an Evaluation's constants are computed in its value type");

    using C  = Opm::Constants<Eval>;
    using W  = Opm::IAPWS::Common<Eval>;
    using CV = Opm::Constants<Value>;
    using WV = Opm::IAPWS::Common<Value>;

    BOOST_CHECK_EQUAL(C::R.value(),    CV::R);
    BOOST_CHECK_EQUAL(C::Na.value(),   CV::Na);
    BOOST_CHECK_EQUAL(C::kb.value(),   CV::kb);
    BOOST_CHECK_EQUAL(C::c.value(),    CV::c);
    BOOST_CHECK_EQUAL(C::h.value(),    CV::h);
    BOOST_CHECK_EQUAL(C::hRed.value(), CV::hRed);

    BOOST_CHECK_EQUAL(W::molarMass.value(),           WV::molarMass);
    BOOST_CHECK_EQUAL(W::Rs.value(),                  WV::Rs);
    BOOST_CHECK_EQUAL(W::criticalDensity.value(),     WV::criticalDensity);
    BOOST_CHECK_EQUAL(W::criticalMolarVolume.value(), WV::criticalMolarVolume);

    for (int i = 0; i < Eval::numVars; ++i) {
        BOOST_CHECK_EQUAL(C::kb.derivative(i),   Value(0));
        BOOST_CHECK_EQUAL(C::hRed.derivative(i), Value(0));
        BOOST_CHECK_EQUAL(W::Rs.derivative(i),   Value(0));
        BOOST_CHECK_EQUAL(W::criticalMolarVolume.derivative(i), Value(0));
    }
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(FloatingPoint)
{
    checkDerivedConstants<float>();
    checkDerivedConstants<double>();
}

BOOST_AUTO_TEST_CASE(Autodiff)
{
    checkAutodiffConstants<Opm::DenseAd::Evaluation<float, 3>>();
    checkAutodiffConstants<Opm::DenseAd::Evaluation<double, 3>>();
}
