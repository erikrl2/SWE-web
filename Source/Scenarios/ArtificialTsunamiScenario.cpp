#include "ArtificialTsunamiScenario.hpp"

#include <cmath>

Scenarios::ArtificialTsunamiScenario::ArtificialTsunamiScenario(double endSimulationTime, BoundaryType boundaryType):
  endSimulationTime_(endSimulationTime),
  boundaryType_(boundaryType) {}

RealType Scenarios::ArtificialTsunamiScenario::getWaterHeight([[maybe_unused]] RealType x, [[maybe_unused]] RealType y) const { return RealType(100.0); }

RealType Scenarios::ArtificialTsunamiScenario::getBathymetry(RealType x, RealType y) const {
  RealType b = -100;
  if (x >= -500.0 && x <= 500.0 && y >= -500.0 && y <= 500.0) {
    RealType dx = std::sin((x / 500.0 + 1) * M_PI);
    RealType dy = -(y / 500.0) * (y / 500.0) + 1.0;
    b += 5.0 * dx * dy;
  }
  return b;
}

RealType Scenarios::ArtificialTsunamiScenario::getBoundaryPos(BoundaryEdge edge) const {
  if (edge == BoundaryEdge::Left) {
    return RealType(-5000.0);
  } else if (edge == BoundaryEdge::Right) {
    return RealType(5000.0);
  } else if (edge == BoundaryEdge::Bottom) {
    return RealType(-5000.0);
  } else {
    return RealType(5000.0);
  }
}
