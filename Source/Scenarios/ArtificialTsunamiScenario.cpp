#include "ArtificialTsunamiScenario.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

Scenarios::ArtificialTsunamiScenario::ArtificialTsunamiScenario(BoundaryType boundaryType):
  boundaryType_(boundaryType) {}

RealType Scenarios::ArtificialTsunamiScenario::getBathymetryBeforeDisplacement([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return -1000; }

RealType Scenarios::ArtificialTsunamiScenario::getDisplacement(RealType x, RealType y) const { return getCustomDisplacement(x, y); }

RealType Scenarios::ArtificialTsunamiScenario::getBoundaryPos(BoundaryEdge edge) const {
  if (edge == BoundaryEdge::Left) {
    return RealType(-1000000.0);
  } else if (edge == BoundaryEdge::Right) {
    return RealType(1000000.0);
  } else if (edge == BoundaryEdge::Bottom) {
    return RealType(-1000000.0);
  } else {
    return RealType(1000000.0);
  }
}

RealType Scenarios::ArtificialTsunamiScenario::getCustomDisplacement(RealType x, RealType y, DisplConfig c) {
  x -= c.offsetX;
  y -= c.offsetY;
  RealType pHalf = 0.5 * c.period;

  RealType b = 0;
  if (x >= -pHalf && x <= pHalf && y >= -pHalf && y <= pHalf && pHalf != 0.0) {
    RealType dx = std::sin((x / pHalf + 1) * M_PI);
    RealType dy = -(y / pHalf) * (y / pHalf) + 1.0;
    b += c.amplitude * dx * dy;
  }
  return b;
}
