/*
  Copyright (C) 2020 Equinor

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

#ifndef OPM_AUQIFER_CONFIG_HPP
#define OPM_AUQIFER_CONFIG_HPP

#include <opm/parser/eclipse/EclipseState/Aquifer/Aquancon.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/Aquifetp.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/AquiferCT.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>

namespace Opm {

class TableManager;
class EclipseGrid;
class Deck;
class FieldPropsManager;

class AquiferConfig {
public:
    AquiferConfig() = default;
    AquiferConfig(const TableManager& tables, const EclipseGrid& grid,
                  const Deck& deck, const FieldPropsManager& field_props);
    AquiferConfig(const Aquifetp& fetp, const AquiferCT& ct, const Aquancon& conn);
    void load_connections(const Deck& deck, const EclipseGrid& grid);


    static AquiferConfig serializeObject();

    bool active() const;
    const AquiferCT& ct() const;
    const Aquifetp& fetp() const;
    const Aquancon& connections() const;
    bool operator==(const AquiferConfig& other);
    bool hasAquifer(const int aquID) const;

    bool hasNumericalAquifer() const;
    bool hasAnalyticalAquifer() const;
    const NumericalAquifers& numericalAquifers() const;
    NumericalAquifers& mutableNumericalAquifers() const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        aquifetp.serializeOp(serializer);
        aquiferct.serializeOp(serializer);
        aqconn.serializeOp(serializer);
        numerical_aquifers.serializeOp(serializer);
    }

private:
    Aquifetp aquifetp;
    AquiferCT aquiferct;
    mutable NumericalAquifers numerical_aquifers;
    Aquancon aqconn;
};

}

#endif
