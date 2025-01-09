#pragma once

#include "Scenario.hpp"

namespace Scenarios {

  class ArtificialTsunamiScenario: public Scenario {
  public:
    ArtificialTsunamiScenario(BoundaryType boundaryType);
    ArtificialTsunamiScenario(const ArtificialTsunamiScenario&) = delete;

    RealType getBathymetryBeforeDisplacement(RealType x, RealType y) const override;
    RealType getDisplacement(RealType x, RealType y) const override;

    BoundaryType getBoundaryType([[maybe_unused]] BoundaryEdge edge) const override { return boundaryType_; }
    RealType     getBoundaryPos(BoundaryEdge edge) const override;

  private:
    BoundaryType boundaryType_;
  };

} // namespace Scenarios
