/**
 * @file TestScenario.hpp
 * @brief For testing purposes.
 */

#pragma once

#include "Scenario.hpp"

namespace Scenarios {

  class TestScenario: public Scenario {
    int n;

  public:
    TestScenario(int n):
      n(n) {}

    RealType getWaterHeight(RealType x, RealType y) const override {
      return x + y; // x,y in [1,n]
    }

    RealType getBoundaryPos(BoundaryEdge edge) const override {
      if (edge == BoundaryEdge::Left || edge == BoundaryEdge::Bottom) {
        return RealType(0.5);
      } else {
        return RealType(n + 0.5);
      }
    }

    double getEndSimulationTime() const override { return RealType(10.0); }

    int getCellCount() { return n; }
  };

} // namespace Scenarios
