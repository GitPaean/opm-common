/*
  Copyright (C) 2020 Equinor ASA
  Copyright (C) 2020 SINTEF Digital

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  ?You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/parser/eclipse/EclipseState/Aqucon.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include "AquiferHelpers.hpp"

#include <string>

namespace Opm {

    NumericalAquiferConnections::NumericalAquiferConnections(const Deck &deck, const EclipseGrid &grid, const std::vector<int>& actnum)
    {
        using AQUCON=ParserKeywords::AQUCON;
        if ( !deck.hasKeyword<AQUCON>() ) return;

        const auto& aqucon_keywords = deck.getKeywordList<AQUCON>();
        for (const auto& keyword : aqucon_keywords) {
            for (const auto& record : *keyword) {
                const auto cons_from_record = NumAquiferCon::generateConnections(grid, record, actnum);
                for (auto con : cons_from_record) {
                    const size_t aqu_id = con.aquifer_id;
                    const size_t global_index = con.global_index;
                    auto& aqu_cons = this->connections_[aqu_id];
                    // TODO: with the following code, we might ignore the situation that a cell is
                    // connected to two different aquifers
                    if (aqu_cons.find(global_index) == aqu_cons.end()) {
                        aqu_cons.insert({global_index, con});
                    } else {
                        std::ostringstream ss;
                        ss << " the cell {" << con.I + 1 << " " << con.J + 1 << " " << con.K + 1 << " } is declared more "
                           << " than once for the Numeri Aquifer " << con.aquifer_id << " \n";
                        throw std::logic_error( ss.str() );
                    }
                }
            }
        }
    }

    const std::map<size_t, NumAquiferCon>&
    NumericalAquiferConnections::getConnections(const size_t aqu_id) const {
        const auto& cons = this->connections_.find(aqu_id);
        if (cons == this->connections_.end())  {
            std::ostringstream ss;
            ss << " Numerical aquifer " << aqu_id << " does not have any connections \n";
            throw std::logic_error( ss.str() );
        } else {
            return cons->second;
        }
    }

    std::vector<NumAquiferCon> NumAquiferCon::generateConnections(const EclipseGrid& grid, const DeckRecord& record,
                                                                  const std::vector<int>& actnum) {
        std::vector<NumAquiferCon> cons;

        using AQUCON = ParserKeywords::AQUCON;
        const size_t aqu_id = record.getItem<AQUCON::ID>().get<int>(0);
        const size_t i1 = record.getItem<AQUCON::I1>().get<int>(0) - 1;
        const size_t j1 = record.getItem<AQUCON::J1>().get<int>(0) -1;
        const size_t k1 = record.getItem<AQUCON::K1>().get<int>(0) - 1;
        const size_t i2 = record.getItem<AQUCON::I2>().get<int>(0) - 1;
        const size_t j2 = record.getItem<AQUCON::J2>().get<int>(0) -1;
        const size_t k2 = record.getItem<AQUCON::K2>().get<int>(0) - 1;

        const std::string str_allow_internal_cells = record.getItem<AQUCON::ALLOW_INTERNAL_CELLS>().getTrimmedString(0);
        // whether the connection face can connect to active/internal cells
        // by default NO, which means basically the aquifer should be outside of the reservoir
        const bool allow_internal_cells = DeckItem::to_bool(str_allow_internal_cells);
        const FaceDir::DirEnum face_dir
                = FaceDir::FromString(record.getItem<AQUCON::CONNECT_FACE>().getTrimmedString(0));
        const double trans_multi = record.getItem<AQUCON::TRANS_MULT>().get<double>(0);
        const int trans_option = record.getItem<AQUCON::TRANS_OPTION>().get<int>(0);
        const double ve_frac_relperm = record.getItem<AQUCON::VEFRAC>().get<double>(0);
        const double ve_frac_cappress = record.getItem<AQUCON::VEFRACP>().get<double>(0);

        for (size_t k = k1; k <= k2; ++k) {
            for (size_t j = j1; j <=j2; ++j) {
                for (size_t i = i1; i <= i2; ++i) {
                    // TODO: we probably should give a message here
/*                    if (!grid.cellActive(i, j, k)) {
                        continue;
                    } */
                    if (!actnum[grid.getGlobalIndex(i, j, k)]) {
                        continue;
                    }
                    if (allow_internal_cells ||
                        !AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, face_dir, actnum)) {
                        const size_t global_index = grid.getGlobalIndex(i, j, k);
                        cons.emplace_back(NumAquiferCon{aqu_id, i, j, k, global_index, face_dir, trans_multi, trans_option,
                                                        allow_internal_cells, ve_frac_relperm, ve_frac_cappress});
                    }
                }
            }
        }
        return cons;
    }
}