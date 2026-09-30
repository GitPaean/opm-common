/*
  Copyright 2026 SINTEF Digital

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

#ifndef OPM_INJECTION_STREAM_HPP
#define OPM_INJECTION_STREAM_HPP

#include <vector>

namespace Opm {

/// Composition of a fluid injected in a compositional run.
///
/// WELLSTRE defines the streams by name.  WINJGAS and WINJOIL give them to
/// wells, and GINJGAS to groups.  A stream is kept under a stream name in one
/// map and under a group name in another, so it carries no name of its own.
class InjectionStream
{
public:
    /// Default constructor.  Mostly for deserialisation.
    InjectionStream() = default;

    /// \param moleFractions One mole fraction per component.
    explicit InjectionStream(std::vector<double> moleFractions);

    /// Create a serialisation test object.
    static InjectionStream serializationTestObject();

    /// One mole fraction per component.
    const std::vector<double>& moleFractions() const { return this->m_mole_fractions; }

    bool operator==(const InjectionStream& other) const = default;

    /// Convert between byte array and object representation.
    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->m_mole_fractions);
    }

private:
    std::vector<double> m_mole_fractions{};
};

} // namespace Opm

#endif // OPM_INJECTION_STREAM_HPP
