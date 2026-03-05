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
 * \brief Implementation of BlackOilFluidState::createFluidState.
 *
 * This file is included at the bottom of BlackOilFluidState.hpp and
 * must not be included directly.
 */

#ifndef OPM_BLACK_OIL_FLUID_STATE_IMPL_HH
#define OPM_BLACK_OIL_FLUID_STATE_IMPL_HH

#include <fmt/format.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace Opm {

template <class ScalarT,
          class FluidSystemT,
          bool storeTemperature,
          bool storeEnthalpy,
          bool enableDissolution,
          bool enableVapwat,
          bool enableBrine,
          bool enableSaltPrecipitation,
          bool enableDissolutionInWater,
          unsigned numStoragePhases>
template <typename ValueType>
auto
BlackOilFluidState<ScalarT, FluidSystemT, storeTemperature, storeEnthalpy,
                   enableDissolution, enableVapwat, enableBrine,
                   enableSaltPrecipitation, enableDissolutionInWater,
                   numStoragePhases>::
createFluidState(const std::vector<ValueType>& fluidComposition,
                 const ValueType& pressure,
                 const ValueType& temperature,
                 Scalar saltConcentration,
                 int pvtRegionIdx,
                 std::string_view name)
{
    static_assert(fluidSystemIsStatic,
                  "BlackOilFluidState::createFluidState requires a stateless "
                  "(static) fluid system");

    using ReturnFluidState =
        BlackOilFluidState<ValueType, FluidSystemT,
                           storeTemperature, storeEnthalpy,
                           enableDissolution, enableVapwat,
                           enableBrine, enableSaltPrecipitation,
                           enableDissolutionInWater, numStoragePhases>;

    ReturnFluidState fluidState;

    if constexpr (storeTemperature) {
        fluidState.setTemperature(temperature);
    }
    if constexpr (enableBrine) {
        fluidState.setSaltConcentration(saltConcentration);
    }

    for (unsigned phaseIdx = 0; phaseIdx < FluidSystemT::numPhases; ++phaseIdx) {
        if (!FluidSystemT::phaseIsActive(phaseIdx)) {
            continue;
        }
        // We assume there is no capillary pressure, so all phases share the
        // same pressure.
        fluidState.setPressure(phaseIdx, pressure);
    }
    fluidState.setPvtRegionIndex(pvtRegionIdx);

    const bool bothOilGas =
        FluidSystemT::phaseIsActive(FluidSystemT::oilPhaseIdx) &&
        FluidSystemT::phaseIsActive(FluidSystemT::gasPhaseIdx);
    const bool bothWaterGas =
        FluidSystemT::phaseIsActive(FluidSystemT::waterPhaseIdx) &&
        FluidSystemT::phaseIsActive(FluidSystemT::gasPhaseIdx);

    const ValueType zeroValue{0.0};

    // -----------------------------------------------------------------------
    // Step 1: Compute dissolution/vaporization factors and inverse formation
    //         volume factors for each active phase.
    // -----------------------------------------------------------------------
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystemT::numPhases; ++phaseIdx) {
        if (!FluidSystemT::phaseIsActive(phaseIdx)) {
            continue;
        }

        const unsigned activeCompIdx =
            FluidSystemT::canonicalToActiveCompIdx(
                FluidSystemT::solventComponentIndex(phaseIdx));

        switch (phaseIdx) {
        case FluidSystemT::oilPhaseIdx: {
            if constexpr (enableDissolution) {
                if (bothOilGas) {
                    // Rs: dissolved gas in oil (m³ gas at surface / m³ oil at surface)
                    const ValueType saturatedRs =
                        FluidSystemT::saturatedDissolutionFactor(
                            fluidState, phaseIdx, fluidState.pvtRegionIndex());
                    const unsigned gasCompIdx =
                        FluidSystemT::canonicalToActiveCompIdx(
                            FluidSystemT::solventComponentIndex(FluidSystemT::gasPhaseIdx));
                    const ValueType maxPossibleRs =
                        fluidComposition[gasCompIdx] / fluidComposition[activeCompIdx];
                    fluidState.setRs(std::min(saturatedRs, maxPossibleRs));
                } else {
                    fluidState.setRs(zeroValue);
                }
            }
            break;
        }

        case FluidSystemT::gasPhaseIdx: {
            if constexpr (enableDissolution) {
                if (bothOilGas) {
                    // Rv: vaporized oil in gas (m³ oil at surface / m³ gas at surface).
                    // saturatedDissolutionFactor for the gas phase returns Rv.
                    const ValueType saturatedRv =
                        FluidSystemT::saturatedDissolutionFactor(
                            fluidState, phaseIdx, fluidState.pvtRegionIndex());
                    const unsigned oilCompIdx =
                        FluidSystemT::canonicalToActiveCompIdx(
                            FluidSystemT::solventComponentIndex(FluidSystemT::oilPhaseIdx));
                    const ValueType maxPossibleRv =
                        fluidComposition[oilCompIdx] / fluidComposition[activeCompIdx];
                    fluidState.setRv(std::min(saturatedRv, maxPossibleRv));
                } else {
                    fluidState.setRv(zeroValue);
                }
            }
            if constexpr (enableVapwat) {
                if (bothWaterGas) {
                    // Rvw: vaporized water in gas (m³ water at surface / m³ gas at surface).
                    // saturatedVaporizationFactor for the gas phase returns Rvw.
                    const ValueType saturatedRvw =
                        FluidSystemT::saturatedVaporizationFactor(
                            fluidState, phaseIdx, fluidState.pvtRegionIndex());
                    const unsigned waterCompIdx =
                        FluidSystemT::canonicalToActiveCompIdx(
                            FluidSystemT::solventComponentIndex(FluidSystemT::waterPhaseIdx));
                    const ValueType maxPossibleRvw =
                        fluidComposition[waterCompIdx] / fluidComposition[activeCompIdx];
                    fluidState.setRvw(std::min(saturatedRvw, maxPossibleRvw));
                } else {
                    fluidState.setRvw(zeroValue);
                }
            }
            break;
        }

        case FluidSystemT::waterPhaseIdx: {
            if constexpr (enableDissolutionInWater) {
                if (bothWaterGas) {
                    // Rsw: dissolved gas in water (m³ gas at surface / m³ water at surface).
                    // saturatedDissolutionFactor for the water phase returns Rsw.
                    const ValueType saturatedRsw =
                        FluidSystemT::saturatedDissolutionFactor(
                            fluidState, phaseIdx, fluidState.pvtRegionIndex());
                    const unsigned gasCompIdx =
                        FluidSystemT::canonicalToActiveCompIdx(
                            FluidSystemT::solventComponentIndex(FluidSystemT::gasPhaseIdx));
                    const ValueType maxPossibleRsw =
                        fluidComposition[gasCompIdx] / fluidComposition[activeCompIdx];
                    fluidState.setRsw(std::min(saturatedRsw, maxPossibleRsw));
                } else {
                    fluidState.setRsw(zeroValue);
                }
            }
            break;
        }

        default:
            throw std::logic_error("Unhandled phase index " + std::to_string(phaseIdx));
        }

        fluidState.setInvB(phaseIdx,
                           FluidSystemT::inverseFormationVolumeFactor(
                               fluidState, phaseIdx, fluidState.pvtRegionIndex()));
    }

    // -----------------------------------------------------------------------
    // Step 2: Compute phase saturations from surface-volume compositions.
    //
    // For water+gas systems with cross-dissolution (Rsw/Rvw active):
    //   q_ws = q_wr * b_w + Rvw * q_gr * b_g
    //   q_gs = q_gr * b_g + Rsw * q_wr * b_w
    //   d    = 1 - Rsw * Rvw
    //   q_wr = (q_ws - Rvw * q_gs) / (b_w * d)
    //   q_gr = (q_gs - Rsw * q_ws) / (b_g * d)
    //
    // For oil+gas systems with dissolution (Rs/Rv active):
    //   q_os = q_or * b_o + Rv * q_gr * b_g
    //   q_gs = q_gr * b_g + Rs * q_or * b_o
    //   d    = 1 - Rs * Rv
    //   q_or = (q_os - Rv * q_gs) / (b_o * d)
    //   q_gr = (q_gs - Rs * q_os) / (b_g * d)
    //
    // For other phases: q_xr = q_xs / b_x
    // -----------------------------------------------------------------------
    const bool bothWaterGasDisgas =
        bothWaterGas &&
        (FluidSystemT::enableDissolvedGasInWater() || FluidSystemT::enableVaporizedWater());

    std::vector<ValueType> saturation(FluidSystemT::numPhases, zeroValue);
    ValueType totalSaturation{0.0};

    for (unsigned phaseIdx = 0; phaseIdx < FluidSystemT::numPhases; ++phaseIdx) {
        if (!FluidSystemT::phaseIsActive(phaseIdx)) {
            continue;
        }

        const bool waterGasDisgasPhase =
            bothWaterGasDisgas &&
            (FluidSystemT::waterPhaseIdx == phaseIdx ||
             FluidSystemT::gasPhaseIdx  == phaseIdx);

        if (waterGasDisgasPhase) {
            const unsigned waterCompIdx =
                FluidSystemT::canonicalToActiveCompIdx(FluidSystemT::waterCompIdx);
            const unsigned gasCompIdx =
                FluidSystemT::canonicalToActiveCompIdx(FluidSystemT::gasCompIdx);
            const ValueType d = 1.0 - fluidState.Rvw() * fluidState.Rsw();
            if (d <= 0.0) {
                throw std::logic_error(
                    fmt::format("Problematic d value {} obtained for {}"
                                " during BlackOilFluidState::createFluidState"
                                " with Rsw {}, Rvw {}.",
                                Opm::getValue(d),
                                name,
                                Opm::getValue(fluidState.Rsw()),
                                Opm::getValue(fluidState.Rvw())));
            }
            if (FluidSystemT::gasPhaseIdx == phaseIdx) {
                saturation[phaseIdx] =
                    (fluidComposition[gasCompIdx] -
                     fluidState.Rsw() * fluidComposition[waterCompIdx]) /
                    (d * fluidState.invB(phaseIdx));
            } else {
                // waterPhaseIdx
                saturation[phaseIdx] =
                    (fluidComposition[waterCompIdx] -
                     fluidState.Rvw() * fluidComposition[gasCompIdx]) /
                    (d * fluidState.invB(phaseIdx));
            }
        } else if (!bothOilGas || FluidSystemT::waterPhaseIdx == phaseIdx) {
            const unsigned activeCompIdx =
                FluidSystemT::canonicalToActiveCompIdx(
                    FluidSystemT::solventComponentIndex(phaseIdx));
            saturation[phaseIdx] =
                fluidComposition[activeCompIdx] / fluidState.invB(phaseIdx);
        } else {
            // Oil and gas phases with mutual dissolution (Rs/Rv)
            const unsigned oilCompIdx =
                FluidSystemT::canonicalToActiveCompIdx(FluidSystemT::oilCompIdx);
            const unsigned gasCompIdx =
                FluidSystemT::canonicalToActiveCompIdx(FluidSystemT::gasCompIdx);
            const ValueType d = 1.0 - fluidState.Rv() * fluidState.Rs();
            if (d <= 0.0) {
                throw std::logic_error(
                    fmt::format("Problematic d value {} obtained for {}"
                                " during BlackOilFluidState::createFluidState"
                                " with Rs {}, Rv {}.",
                                Opm::getValue(d),
                                name,
                                Opm::getValue(fluidState.Rs()),
                                Opm::getValue(fluidState.Rv())));
            }
            if (FluidSystemT::gasPhaseIdx == phaseIdx) {
                saturation[phaseIdx] =
                    (fluidComposition[gasCompIdx] -
                     fluidState.Rs() * fluidComposition[oilCompIdx]) /
                    (d * fluidState.invB(phaseIdx));
            } else {
                // oilPhaseIdx
                saturation[phaseIdx] =
                    (fluidComposition[oilCompIdx] -
                     fluidState.Rv() * fluidComposition[gasCompIdx]) /
                    (d * fluidState.invB(phaseIdx));
            }
        }

        totalSaturation += saturation[phaseIdx];
    }

    // -----------------------------------------------------------------------
    // Step 3: Normalize saturations and compute phase densities (and
    //         enthalpies when the energy equation is active).
    // -----------------------------------------------------------------------
    for (unsigned phaseIdx = 0; phaseIdx < FluidSystemT::numPhases; ++phaseIdx) {
        if (!FluidSystemT::phaseIsActive(phaseIdx)) {
            continue;
        }

        fluidState.setSaturation(phaseIdx, saturation[phaseIdx] / totalSaturation);
        fluidState.setDensity(phaseIdx,
                              FluidSystemT::density(
                                  fluidState, phaseIdx, fluidState.pvtRegionIndex()));

        if constexpr (storeEnthalpy) {
            fluidState.setEnthalpy(phaseIdx,
                                   FluidSystemT::enthalpy(
                                       fluidState, phaseIdx, fluidState.pvtRegionIndex()));
        }
    }

    return fluidState;
}

} // namespace Opm

#endif // OPM_BLACK_OIL_FLUID_STATE_IMPL_HH
