#ifndef OPM_EQUIL_HPP
#define OPM_EQUIL_HPP

#include <cstddef>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;
    class KeywordLocation;
    class Phases;

    /// Single equilibration region record from the EQUIL keyword.
    ///
    /// Holds datum depth, contact depths, capillary pressures, and
    /// initialisation options for one EQUIL region.  Items 10 and 11 are
    /// specific to compositional (E300-style) simulations and are only
    /// populated when the model is run in compositional mode.
    class EquilRecord {
        public:
            EquilRecord() = default;
            EquilRecord(double datum_depth_arg, double datum_depth_pc_arg,
                        double woc_depth, double woc_pc,
                        double goc_depth, double goc_pc,
                        bool live_oil_init,
                        bool wet_gas_init,
                        int target_accuracy,
                        bool humid_gas_init);
            EquilRecord(const DeckRecord& record, const Phases& phases, int region, const KeywordLocation& location, bool compositional);

            static EquilRecord serializationTestObject();

            /// Item 1 – Datum depth for the equilibration region.
            double datumDepth() const;

            /// Item 2 – Pressure at the datum depth.
            double datumDepthPressure() const;

            /// Item 3 – Depth of the water-oil contact (OWC).
            double waterOilContactDepth() const;

            /// Item 4 – Capillary pressure at the water-oil contact.
            double waterOilContactCapillaryPressure() const;

            /// Item 5 – Depth of the gas-oil contact (GOC).
            double gasOilContactDepth() const;

            /// Item 6 – Capillary pressure at the gas-oil contact.
            double gasOilContactCapillaryPressure() const;

            /// Item 7 – Whether to use a constant Rs (live-oil) initialisation
            /// procedure.  True when the deck value is <= 0.
            bool liveOilInitConstantRs() const;

            /// Item 8 – Whether to use a constant Rv (wet-gas) initialisation
            /// procedure.  True when the deck value is <= 0.
            bool wetGasInitConstantRv() const;

            /// Item 9 – Accuracy/method indicator for the initial fluid-in-place
            /// calculation (OIP_INIT).  Defaults to -5 for black-oil runs
            /// and 0 for compositional runs.
            int initializationTargetAccuracy() const;

            /// Item 10 – Compositional initialisation type (E300-specific).
            /// Only meaningful in compositional simulations.  Typical values:
            ///   1 – default compositional init,
            ///   2 – alternative init with optional saturation-pressure override,
            ///   3 – alternative init with optional saturation-pressure override.
            int compositionalInitType() const;

            /// Item 11 – Whether the field pressure is set to the saturation
            /// pressure at the contact depth (E300-specific).  Only relevant
            /// when compositionalInitType() returns 2 or 3.  True (the
            /// default) means saturation pressure is applied; false means it
            /// is not.
            bool setToSaturaionPressure() const;

            /// Item 12 – Whether to use a constant Rvw (humid-gas)
            /// initialisation procedure.  True when the deck value is <= 0.
            bool humidGasInitConstantRvw() const;

            bool operator==(const EquilRecord& data) const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(datum_depth);
                serializer(datum_depth_ps);
                serializer(water_oil_contact_depth);
                serializer(water_oil_contact_capillary_pressure);
                serializer(gas_oil_contact_depth);
                serializer(gas_oil_contact_capillary_pressure);
                serializer(live_oil_init_proc);
                serializer(wet_gas_init_proc);
                serializer(init_target_accuracy);
                serializer(comp_init_type);
                serializer(set_to_saturaion_pressure);
                serializer(humid_gas_init_proc);
            }

        private:
            /// Item 1 – Datum depth for the equilibration region.
            double datum_depth = 0.0;

            /// Item 2 – Pressure at the datum depth.
            double datum_depth_ps = 0.0;

            /// Item 3 – Depth of the water-oil contact.
            double water_oil_contact_depth = 0.0;

            /// Item 4 – Capillary pressure at the water-oil contact.
            double water_oil_contact_capillary_pressure = 0.0;

            /// Item 5 – Depth of the gas-oil contact.
            double gas_oil_contact_depth = 0.0;

            /// Item 6 – Capillary pressure at the gas-oil contact.
            double gas_oil_contact_capillary_pressure = 0.0;

            /// Item 7 – Live-oil (constant Rs) initialisation flag.
            bool live_oil_init_proc = false;

            /// Item 8 – Wet-gas (constant Rv) initialisation flag.
            bool wet_gas_init_proc = false;

            /// Item 9 – Initialisation target accuracy / method (OIP_INIT).
            /// Defaults to -5 for black-oil, 0 for compositional.
            int init_target_accuracy = 0;

            /// Item 10 – Compositional initialisation type (E300-specific).
            /// Only populated in compositional simulations.
            int comp_init_type = 0;

            /// Item 11 – Whether to set the field pressure to the saturation
            /// pressure at the contact depth.  Only relevant when
            /// comp_init_type is 2 or 3 (E300-specific, compositional only).
            bool set_to_saturaion_pressure = true;

            /// Item 12 – Humid-gas (constant Rvw) initialisation flag.
            bool humid_gas_init_proc = false;
    };

    class StressEquilRecord {
        public:
            StressEquilRecord() = default;
            StressEquilRecord(const DeckRecord& record, const Phases& phases, int region, const KeywordLocation& location, bool compositional);

            static StressEquilRecord serializationTestObject();

            bool operator==(const StressEquilRecord& data) const;

            double datumDepth() const;
            double datumPosX() const;
            double datumPosY() const;
            double stressXX() const;
            double stressXX_grad() const;
            double stressYY() const;
            double stressYY_grad() const;
            double stressZZ() const;
            double stressZZ_grad() const;

            double stressXY() const;
            double stressXY_grad() const;
            double stressXZ() const;
            double stressXZ_grad() const;
            double stressYZ() const;
            double stressYZ_grad() const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(datum_depth);
                serializer(datum_posx);
                serializer(datum_posy);
                serializer(stress_xx);
                serializer(stress_xx_grad);
                serializer(stress_yy);
                serializer(stress_yy_grad);
                serializer(stress_zz);
                serializer(stress_zz_grad);

                serializer(stress_xy);
                serializer(stress_xy_grad);
                serializer(stress_xz);
                serializer(stress_xz_grad);
                serializer(stress_yz);
                serializer(stress_yz_grad);
            }

        private:
            double datum_depth = 0.0;
            double datum_posx = 0.0;
            double datum_posy = 0.0;
            double stress_xx = 0.0;
            double stress_xx_grad = 0.0;
            double stress_yy = 0.0;
            double stress_yy_grad = 0.0;
            double stress_zz = 0.0;
            double stress_zz_grad = 0.0;

            double stress_xy = 0.0;
            double stress_xy_grad = 0.0;
            double stress_xz = 0.0;
            double stress_xz_grad = 0.0;
            double stress_yz = 0.0;
            double stress_yz_grad = 0.0;
    };

    template<class RecordType>
    class EquilContainer {
        public:
            using const_iterator = typename std::vector<RecordType>::const_iterator;

            EquilContainer() = default;
            EquilContainer( const DeckKeyword&, const Phases&, bool );

            static EquilContainer serializationTestObject();

            const RecordType& getRecord(std::size_t id) const;

            size_t size() const;
            bool empty() const;

            const_iterator begin() const;
            const_iterator end() const;

            bool operator==(const EquilContainer& data) const;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(m_records);
            }

        private:
            std::vector<RecordType> m_records;
    };

    using Equil = EquilContainer<EquilRecord>;
    using StressEquil = EquilContainer<StressEquilRecord>;
}

#endif //OPM_EQUIL_HPP
