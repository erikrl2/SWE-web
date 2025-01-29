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

  public:
    struct DisplConfig {
      RealType amplitude;
      RealType period;
      RealType offsetX;
      RealType offsetY;
    };
    static inline const DisplConfig defaultDisplConfig{10.0, 200000.0, 0.0, 0.0};

    static RealType getCustomDisplacement(RealType x, RealType y, DisplConfig c = defaultDisplConfig);

  private:
    BoundaryType boundaryType_;
  };

} // namespace Scenarios
