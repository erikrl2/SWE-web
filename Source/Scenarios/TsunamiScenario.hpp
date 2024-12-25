#pragma once

#ifndef __EMSCRIPTEN__
#include <string>

#include "Scenario.hpp"
#include "Types/Float2D.hpp"

namespace Scenarios {

  /**
   * Scenario "Tsunami":
   */
  class TsunamiScenario: public Scenario {
  public:
    TsunamiScenario(const std::string& bathymetryFile, const std::string& displacementFile, BoundaryType boundaryType, int nX, int nY);
    ~TsunamiScenario() override = default;

    RealType getWaterHeight(RealType x, RealType y) const override;
    RealType getBathymetry(RealType x, RealType y) const override;

    BoundaryType getBoundaryType(BoundaryEdge) const override { return boundaryType_; }
    RealType     getBoundaryPos(BoundaryEdge edge) const override { return boundaryPos_[edge]; }

    bool success() const { return success_; }

  private:
    RealType getBathymetryBeforeEarthquake(RealType x, RealType y) const;
    RealType getDisplacement(RealType x, RealType y) const;

  private:
    BoundaryType boundaryType_;
    int          nX_, nY_;
    RealType     dX_, dY_;

    Float2D<RealType> b_;
    int               bNX_, bNY_;
    double            boundaryPos_[4];
    RealType          originX_, originY_;
    RealType          bDX_, bDY_;

    Float2D<RealType> d_;
    int               dNX_, dNY_;
    double            dBoundaryPos_[4];
    RealType          dOriginX_, dOriginY_;
    RealType          dDX_, dDY_;

    bool success_ = true;
  };

} // namespace Scenarios
#endif
