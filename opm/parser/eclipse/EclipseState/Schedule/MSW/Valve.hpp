/*
  Copyright 2019 Equinor.

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

#ifndef VALVE_HPP_HEADER_INCLUDED
#define VALVE_HPP_HEADER_INCLUDED

#include <map>
#include <utility>
#include <vector>


namespace Opm {

    class DeckRecord;
    class DeckKeyword;

    class Valve {
    public:
        enum class Status {
            OPEN,
            SHUT
        };

        explicit Valve(const DeckRecord& record);

        // the function will return a map
        // [
        //     "WELL1" : [<seg1, valv1>, <seg2, valv2> ...]
        //     ....
        static std::map<std::string, std::vector<std::pair<int, Valve> > >
        fromWSEGVAVLV(const DeckKeyword& keyword);

        Status status() const;
        double flowCoefficient() const;
        double crossArea() const;
        double additionalLength() const;
        double pipeDiameter() const;
        double absRoughness() const;
        double pipeCrossArea() const;
        double maxCrossArea() const;

        void setPipeDiameter(const double dia);
        void setAbsRoughness(const double rou);
        void setPipeCrossArea(const double area);
        void setMaxCrossArea(const double area);
        void setAdditionalLength(const double length);

    private:
        double m_flow_coef;
        double m_cross_area;
        double m_additional_length;
        double m_pipe_diameter;
        double m_abs_roughness;
        double m_pipe_cross_area;
        Status m_status;
        double m_max_cross_area;
    };

}

#endif
