/*
  Copyright (C) 2023 Equinor

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

#ifndef OPM_AQUIFERCONSTANTFLUX_HPP
#define OPM_AQUIFERCONSTANTFLUX_HPP

#include <optional>
#include <unordered_map>

namespace Opm {
    class Deck;
    class DeckRecord;
}

namespace  Opm {
    struct SingleAquiferConstantFlux {
        explicit SingleAquiferConstantFlux(const DeckRecord& record);
        int id;
        double aquifer_flux;
        double salt_concentration;
        std::optional<double> temperature;
        std::optional<double> datum_pressure;
        int name() {
            return id;
        }
    };


    class AquiferConstantFlux {
    public:
        // not sure whether we should use Deck as the argument
        // remaining to be revised
        AquiferConstantFlux() = default;
        explicit AquiferConstantFlux(const Deck& deck);
        void handleAQUFLUX(const DeckRecord& record);
    private:
        std::unordered_map<int, SingleAquiferConstantFlux> m_aquifers;
    };
} // end of namespace Opm

#endif //OPM_AQUIFERCONSTANTFLUX_HPP
