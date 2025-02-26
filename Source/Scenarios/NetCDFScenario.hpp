#pragma once

#ifdef ENABLE_NETCDF
#include <string>

#include "Scenario.hpp"
#include "Types/Float2D.hpp"

namespace Scenarios {

  class NetCDFScenario: public Scenario {
  public:
    NetCDFScenario(const std::string& bathymetryFile, const std::string& displacementFile, BoundaryType boundaryType);
    ~NetCDFScenario() override = default;

    RealType getBathymetryBeforeDisplacement(RealType x, RealType y) const override;
    RealType getDisplacement(RealType x, RealType y) const override;

    BoundaryType getBoundaryType(BoundaryEdge) const override { return boundaryType_; }
    RealType     getBoundaryPos(BoundaryEdge edge) const override { return boundaryPos_[edge]; }

    bool loadSuccess() const override { return success_; }

  private:
    BoundaryType boundaryType_;

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

    bool noDisplacement_ = false;

    bool success_ = true;
  };

} // namespace Scenarios
#endif
