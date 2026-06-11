// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2023 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP
#define OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp> // TODO: this is something else need to check
#include <opm/material/viscositymodels/LBC.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

namespace Opm {

/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase system that can contain NumComp components
 *
 * \tparam Scalar  The floating-point type that specifies the precision of the numerical operations.
 * \tparam NumComp The number of the components in the fluid system.
 */
    template<class Scalar, int NumComp, bool enableWater>
    class GenericOilGasWaterFluidSystem
        : public BaseFluidSystem<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr bool waterEnabled = enableWater;
        static constexpr int numPhases = enableWater ? 3 : 2;
        static constexpr int numComponents = NumComp;
        static constexpr int numMisciblePhases = 2;
        // \Note: not totally sure when we should distinguish numMiscibleComponents and numComponents.
        // Possibly when with a dummy phase like water?
        static constexpr int numMiscibleComponents = NumComp;
        // TODO: phase location should be more general
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;
        static constexpr int waterPhaseIdx = 2;

        static constexpr int waterCompIdx = -1;
        static constexpr int oilCompIdx = 0;
        static constexpr int gasCompIdx = 1;
        static constexpr int compositionSwitchIdx = -1; // equil initializer

        template <class ValueType>
        using ParameterCache = Opm::PTFlashParameterCache<ValueType, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using ViscosityModel = Opm::ViscosityModels<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using CubicEOS = Opm::CubicEOS<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using WaterPvt = WaterPvtMultiplexer<Scalar>;
        using EOSType = CompositionalConfig::EOSType;

        struct ComponentParam {
            std::string name;
            Scalar molar_mass; // unit: g/mol
            Scalar critic_temp; // unit: K
            Scalar critic_pres; // unit: parscal
            Scalar critic_vol; // unit: m^3/kmol
            Scalar acentric_factor; // unit: dimension less

            ComponentParam(const std::string_view name_, const Scalar molar_mass_, const Scalar critic_temp_,
                           const Scalar critic_pres_, const Scalar critic_vol_, const Scalar acentric_factor_)
                    : name(name_),
                      molar_mass(molar_mass_),
                      critic_temp(critic_temp_),
                      critic_pres(critic_pres_),
                      critic_vol(critic_vol_),
                      acentric_factor(acentric_factor_)
            {}
        };

        static bool phaseIsActive(unsigned phaseIdx)
        {
            if constexpr (enableWater) {
                assert(phaseIdx < numPhases);
                return true;
            }
            else {
                if (phaseIdx == waterPhaseIdx)
                    return false;
                assert(phaseIdx < numPhases);
                return true;
            }
        }

        template<typename Param>
        static void addComponent(const Param& param, unsigned regionIdx = 0)
        {
            // allow single-region setups to add components without calling init() first
            if (component_param_.empty() && regionIdx == 0) {
                component_param_.resize(1);
                component_param_[0].reserve(numComponents);
                interaction_coefficients_.resize(1);
                eos_types_.assign(1, EOSType::PR);
            }
            assert(regionIdx < component_param_.size());
            // Check if the current size is less than the maximum allowed components.
            if (component_param_[regionIdx].size() < numComponents) {
                component_param_[regionIdx].push_back(param);
            } else {
                // Adding another component would exceed the limit.
                const std::string msg = fmt::format("The fluid system has reached its maximum capacity of {} components,"
                                                    "the component '{}' will not be added.", NumComp, param.name);
                OpmLog::note(msg);
                // Optionally, throw an exception?
            }
        }

        /*!
         * \brief Initialize the fluid system using an ECL deck object
         */
        static void initFromState(const EclipseState& eclState, const Schedule& schedule)
        {
            const auto& comp_config = eclState.compositionalConfig();
            // how should we utilize the numComps from the CompositionalConfig?
            using FluidSystem = GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>;
            const std::size_t num_comps = comp_config.numComps();
            assert(num_comps == NumComp);
            const std::size_t num_regions = comp_config.numEosRegions();
            const auto& names = comp_config.compName();
            FluidSystem::init(num_regions);
            using CompParm = typename FluidSystem::ComponentParam;
            for (std::size_t region = 0; region < num_regions; ++region) {
                FluidSystem::setEOSType(comp_config.eosType(region), region);
                const auto& molar_weight = comp_config.molecularWeights(region);
                const auto& acentric_factor = comp_config.acentricFactors(region);
                const auto& critic_pressure = comp_config.criticalPressure(region);
                const auto& critic_temp = comp_config.criticalTemperature(region);
                const auto& critic_volume = comp_config.criticalVolume(region);
                for (std::size_t c = 0; c < num_comps; ++c) {
                    // we use m^3/kmol for the critic volume in the flash calculation, so we multiply 1.e3 for the critic volume
                    FluidSystem::addComponent(CompParm{names[c], molar_weight[c], critic_temp[c], critic_pressure[c],
                                                       critic_volume[c] * 1.e3, acentric_factor[c]},
                                              region);
                }

                const auto& bic = comp_config.binaryInteractionCoefficient(region);
                interaction_coefficients_[region].resize(bic.size());
                std::ranges::copy(bic, interaction_coefficients_[region].begin());

                // the average molar mass of a phase is calculated through the fluid state without
                // knowing the EOS region. until that is made region-aware, region-dependent molecular
                // weights would give inconsistent phase densities, so we warn about it here
                if (region > 0 && comp_config.molecularWeights(region) != comp_config.molecularWeights(0)) {
                    OpmLog::warning(fmt::format("Molecular weights of EOS region {} differ from EOS region 1. "
                                                "The average molar mass calculation uses the values of EOS region 1.",
                                                region + 1));
                }
            }

            // Init. water pvt from deck
            waterPvt_->initFromState(eclState, schedule);

        }

        /*!
         * \brief Prepare the fluid system for a single-region setup without
         *        discarding components that were already added.
         */
        static void init()
        {
            waterPvt_ = std::make_shared<WaterPvt>();
            if (component_param_.empty()) {
                component_param_.resize(1);
                interaction_coefficients_.resize(1);
                // PR matches the EOS type previously assumed when no deck information was available
                eos_types_.assign(1, EOSType::PR);
            }
            for (auto& params : component_param_) {
                params.reserve(numComponents);
            }
        }

        /*!
         * \brief Reset the fluid system to hold parameters for the given number of EOS regions.
         */
        static void init(std::size_t numRegions)
        {
            waterPvt_ = std::make_shared<WaterPvt>();
            component_param_.assign(numRegions, {});
            for (auto& params : component_param_) {
                params.reserve(numComponents);
            }
            interaction_coefficients_.assign(numRegions, {});
            eos_types_.assign(numRegions, EOSType::PR);
        }

        /*!
         * \brief The number of EOS regions the fluid system holds parameters for.
         */
        static std::size_t numRegions()
        { return component_param_.size(); }

        static void setEOSType(EOSType eos_type, unsigned regionIdx = 0)
        {
            assert(regionIdx < eos_types_.size());
            eos_types_[regionIdx] = eos_type;
        }

        /*!
         * \brief The equation of state to be used for a given EOS region.
         */
        static EOSType eosType(unsigned regionIdx = 0)
        {
            assert(regionIdx < eos_types_.size());
            return eos_types_[regionIdx];
        }

        /*!
        * \brief Set the pressure-volume-saturation (PVT) relations for the water phase.
        */
        static void setWaterPvt(std::shared_ptr<WaterPvt> pvtObj)
        { waterPvt_ = std::move(pvtObj); }

        /*!
         * \brief The acentric factor of a component [].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar acentricFactor(unsigned compIdx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);
            assert(regionIdx < component_param_.size());

            return component_param_[regionIdx][compIdx].acentric_factor;
        }
        /*!
         * \brief Critical temperature of a component [K].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalTemperature(unsigned compIdx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);
            assert(regionIdx < component_param_.size());

            return component_param_[regionIdx][compIdx].critic_temp;
        }
        /*!
         * \brief Critical pressure of a component [Pa].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalPressure(unsigned compIdx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);
            assert(regionIdx < component_param_.size());

            return component_param_[regionIdx][compIdx].critic_pres;
        }
        /*!
        * \brief Critical volume of a component [m3].
        *
        * \copydetails Doxygen::compIdxParam
        */
        static Scalar criticalVolume(unsigned compIdx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);
            assert(regionIdx < component_param_.size());

            return component_param_[regionIdx][compIdx].critic_vol;
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);
            assert(regionIdx < component_param_.size());

            return component_param_[regionIdx][compIdx].molar_mass;
        }

        /*!
         * \brief Returns the interaction coefficient for two components.
         *.
         */
        static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx, unsigned regionIdx = 0)
        {
            assert(isConsistent());
            assert(comp1Idx < numComponents);
            assert(comp2Idx < numComponents);
            assert(regionIdx < interaction_coefficients_.size());
            if (interaction_coefficients_[regionIdx].empty() || comp2Idx == comp1Idx) {
                return 0.0;
            }
            // make sure row is the bigger value compared to column number
            const auto [column, row] = std::minmax(comp1Idx, comp2Idx);
            const unsigned index = (row * (row - 1) / 2 + column); // it is the current understanding
            return interaction_coefficients_[regionIdx][index];
        }

        //! \copydoc BaseFluidSystem::phaseName
        static std::string_view phaseName(unsigned phaseIdx)
        {
                static const std::string_view name[] = {"o",  // oleic phase
                                                        "g",  // gas phase
                                                        "w"};  // aqueous phase

                assert(phaseIdx < numPhases);
                return name[phaseIdx];
        }

        //! \copydoc BaseFluidSystem::componentName
        static std::string_view componentName(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            // the component names are the same for all EOS regions
            return component_param_[0][compIdx].name;
        }

        /*!
         * \copydoc BaseFluidSystem::density
         */
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval density(const FluidState& fluidState,
                               const ParameterCache<ParamCacheEval>& paramCache,
                               unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);

            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                return decay<LhsEval>(fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx));
            }
            else {
                const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
                const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
                const Scalar& rho_w_ref = waterPvt_->waterReferenceDensity(0);
                const LhsEval& bw = waterPvt_->inverseFormationVolumeFactor(0, T, p, LhsEval(0.0), LhsEval(0.0));
                return rho_w_ref * bw;
            }

            return {};
        }

        //! \copydoc BaseFluidSystem::viscosity
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval viscosity(const FluidState& fluidState,
                                 const ParameterCache<ParamCacheEval>& paramCache,
                                 unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);

            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                // Use LBC method to calculate viscosity
                return decay<LhsEval>(ViscosityModel::LBC(fluidState, paramCache, phaseIdx));
            }
            else {
                const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
                const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
                return waterPvt_->viscosity(0, T, p, LhsEval(0.0), LhsEval(0.0));
            }
        }

        //! \copydoc BaseFluidSystem::fugacityCoefficient
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                           const ParameterCache<ParamCacheEval>& paramCache,
                                           unsigned phaseIdx,
                                           unsigned compIdx)
        {
            if (waterEnabled && phaseIdx == static_cast<unsigned int>(waterPhaseIdx))
                return LhsEval(0.0);

            assert(isConsistent());
            assert(phaseIdx < numPhases);
            assert(compIdx < numComponents);

            return decay<LhsEval>(CubicEOS::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx));
        }

        // TODO: the following interfaces are needed by function checkFluidSystem()
        //! \copydoc BaseFluidSystem::isCompressible
        static bool isCompressible([[maybe_unused]] unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return true;
        }

        //! \copydoc BaseFluidSystem::isIdealMixture
        static bool isIdealMixture([[maybe_unused]] unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return false;
        }

        //! \copydoc BaseFluidSystem::isLiquid
        static bool isLiquid(unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return (phaseIdx == oilPhaseIdx);
        }

        //! \copydoc BaseFluidSystem::isIdealGas
        static bool isIdealGas(unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return (phaseIdx == gasPhaseIdx);
        }
        // the following funcitons are needed to compile the GenericOutputBlackoilModule
        // not implemented for this FluidSystem yet
        template <class LhsEval>
        static LhsEval convertXwGToxwG(const LhsEval&, unsigned)
        {
            assert(false && "convertXwGToxwG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }
        template <class LhsEval>
        static LhsEval convertXoGToxoG(const LhsEval&, unsigned)
        {
            assert(false && "convertXoGToxoG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertxoGToXoG(const LhsEval&, unsigned)
        {
            assert(false && "convertxoGToXoG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertXgOToxgO(const LhsEval&, unsigned)
        {
            assert(false && "convertXgOToxgO not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertRswToXwG(const LhsEval&, unsigned)
        {
            assert(false && "convertRswToXwG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertRvwToXgW(const LhsEval&, unsigned)
        {
            assert(false && "convertRvwToXgW not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertXgWToxgW(const LhsEval&, unsigned)
        {
            assert(false && "convertXgWToxgW not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        static bool enableDissolvedGas()
        {
            return false;
        }

        static bool enableDissolvedGasInWater()
        {
            return false;
        }

        static bool enableVaporizedWater()
        {
            return false;
        }

        static bool enableVaporizedOil()
        {
            return false;
        }

    private:
        static bool isConsistent() {
            return !component_param_.empty() &&
                   std::all_of(component_param_.begin(), component_param_.end(),
                               [](const auto& params) { return params.size() == NumComp; });
        }

        // the following members are stored per EOS region
        static std::vector<std::vector<ComponentParam>> component_param_;
        static std::vector<std::vector<Scalar>> interaction_coefficients_;
        static std::vector<EOSType> eos_types_;
        static std::shared_ptr<WaterPvt> waterPvt_;

    public:
        static std::string printComponentParams() {
            std::string result = "Components Information:\n";
            for (std::size_t region = 0; region < component_param_.size(); ++region) {
                result += fmt::format("EOS region {}:\n", region + 1);
                for (const auto& param : component_param_[region]) {
                    result += fmt::format("Name: {}\n", param.name);
                    result += fmt::format("Molar Mass: {} g/mol\n", param.molar_mass);
                    result += fmt::format("Critical Temperature: {} K\n", param.critic_temp);
                    result += fmt::format("Critical Pressure: {} Pascal\n", param.critic_pres);
                    result += fmt::format("Critical Volume: {} m^3/kmol\n", param.critic_vol);
                    result += fmt::format("Acentric Factor: {}\n", param.acentric_factor);
                    result += "---------------------------------\n";
                }
            }
            return result;
        }
    };

    template <class Scalar, int NumComp, bool enableWater>
    std::vector<std::vector<typename GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::ComponentParam>>
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::component_param_;

    template <class Scalar, int NumComp, bool enableWater>
    std::vector<std::vector<Scalar>>
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::interaction_coefficients_;

    template <class Scalar, int NumComp, bool enableWater>
    std::vector<typename GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::EOSType>
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::eos_types_;

    template <class Scalar, int NumComp, bool enableWater>
    std::shared_ptr<WaterPvtMultiplexer<Scalar> >
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::waterPvt_;

}
#endif // OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP
