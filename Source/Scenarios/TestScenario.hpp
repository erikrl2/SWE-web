/**
 * @file TestScenario.hpp
 * @brief For testing purposes.
 */

#pragma once

#include "Scenario.hpp"

namespace Scenarios {

  class TestScenario: public Scenario {
  public:
    TestScenario(BoundaryType boundaryType, int n):
      boundaryType_(boundaryType),
      n_(n) {}

    TestScenario(const TestScenario& other) = delete;

    RealType getWaterHeight(RealType x, RealType y) const override { return x + y; }

    RealType getBoundaryPos(BoundaryEdge edge) const override {
      if (edge == BoundaryEdge::Left || edge == BoundaryEdge::Bottom) {
        return RealType(0.5);
      } else {
        return RealType(n_ + 0.5);
      }
    }

    BoundaryType getBoundaryType([[maybe_unused]] BoundaryEdge edge) const override { return boundaryType_; }

  private:
    BoundaryType boundaryType_;
    int          n_;
  };

} // namespace Scenarios
