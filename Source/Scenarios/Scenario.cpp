/**
 * @file
 * This file is part of SWE.
 *
 * @author Michael Bader, Kaveh Rahnema, Tobias Schnabel
 *
 * @section LICENSE
 *
 * SWE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SWE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SWE.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Scenario.hpp"

#include <cmath>

RealType Scenarios::Scenario::getWaterHeight([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const {
  return -std::fmin(getBathymetryBeforeDisplacement(x, y), RealType(0.0));
}

RealType Scenarios::Scenario::getMomentumU([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return RealType(0.0); }

RealType Scenarios::Scenario::getMomentumV([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return RealType(0.0); }

RealType Scenarios::Scenario::getBathymetry([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const {
  return getBathymetryBeforeDisplacement(x, y) + getDisplacement(x, y);
}

RealType Scenarios::Scenario::getBathymetryBeforeDisplacement([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return RealType(-10.0); }

RealType Scenarios::Scenario::getDisplacement([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return RealType(0.0); }

BoundaryType Scenarios::Scenario::getBoundaryType([[maybe_unused]] BoundaryEdge edge) const { return BoundaryType::Wall; }

RealType Scenarios::Scenario::getBoundaryPos(BoundaryEdge edge) const {
  if (edge == BoundaryEdge::Left || edge == BoundaryEdge::Bottom) {
    return RealType(0.0);
  } else {
    return RealType(100.0);
  }
}

bool Scenarios::Scenario::loadSuccess() const { return true; }
