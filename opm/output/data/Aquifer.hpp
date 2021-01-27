/*
  Copyright 2019 Statoil ASA.

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

#ifndef OPM_OUTPUT_AQUIFER_HPP
#define OPM_OUTPUT_AQUIFER_HPP

#include <map>
#include <memory>
#include <vector>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm { namespace data {

    /**
     * Small struct that keeps track of data for output to restart/summary
     * files.
     */
    enum class AquiferType
    {
        Fetkovich, CarterTracey, Numerical,
    };

    struct FetkovichData {
        double initVolume;
        double prodIndex;
        double timeConstant;
    };

    struct AquiferData {
        int aquiferID = 0;    //< One-based ID, range 1..NANAQ
        double pressure = 0.0;  //< Aquifer pressure
        double fluxRate = 0.0; //< Aquifer influx rate (liquid aquifer)
        // TODO: volume should have a better name, since meaning not clear
        double volume = 0.0;    //< Produced liquid volume
        double initPressure = 0.0;    //< Aquifer's initial pressure
        double datumDepth = 0.0;      //< Aquifer's pressure reference depth

        AquiferType type;
        std::shared_ptr<FetkovichData> aquFet;

        double get(const std::string& key) const
        {
            if ( key == "AAQR" || key == "ANQR" ) {
                return this->fluxRate;
            } else if ( key == "AAQT" || key == "ANQT" ) {
                return this->volume;
            } else if ( key == "AAQP" || key == "ANQP" ) {
                return this->pressure;
            }
            return 0.;
        }

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const {
            buffer.write(aquiferID);
            buffer.write(pressure);
            buffer.write(fluxRate);
            buffer.write(volume);
            buffer.write(initPressure);
            buffer.write(datumDepth);
            int aqu = aquFet ? 1 : 0;
            buffer.write(aqu);
            if (aquFet) {
                buffer.write(aquFet->initVolume);
                buffer.write(aquFet->prodIndex);
                buffer.write(aquFet->timeConstant);
            }
        }

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer) {
            buffer.read(aquiferID);
            buffer.read(pressure);
            buffer.read(fluxRate);
            buffer.read(volume);
            buffer.read(initPressure);
            buffer.read(datumDepth);
            int aqu;
            buffer.read(aqu);
            if (aqu == 1) {
                if (!aquFet)
                    aquFet = std::make_shared<FetkovichData>();
                buffer.read(aquFet->initVolume);
                buffer.read(aquFet->prodIndex);
                buffer.read(aquFet->timeConstant);
            }
        }
    };

    // TODO: not sure what extension we will need
    using Aquifers = std::map<int, AquiferData>;

}} // Opm::data

#endif // OPM_OUTPUT_AQUIFER_HPP
