#pragma once

#include <cstdint>
#include <string>

#include "Scenario.hpp"
#include "Types/Float2D.hpp"

namespace Scenarios {

  enum class RealisticScenarioType {
    Tohoku,
  };

  class RealisticScenario: public Scenario {
  public:
    RealisticScenario(RealisticScenarioType scenario, BoundaryType boundaryType);
    ~RealisticScenario() override = default;

    RealType getBathymetryBeforeDisplacement(RealType x, RealType y) const override;
    RealType getDisplacement(RealType x, RealType y) const override;

    BoundaryType getBoundaryType(BoundaryEdge) const override { return boundaryType_; }
    RealType     getBoundaryPos(BoundaryEdge edge) const override { return boundaryPos_[edge]; }

    bool loadSuccess() const override { return success_; }

  private:
    struct FileHeader {
      uint32_t nX;
      uint32_t nY;
      double   originX;
      double   originY;
      double   dx;
      double   dy;
    };

    bool loadBinaryData(const std::string& filename, FileHeader& header, Float2D<double>& data);

    BoundaryType boundaryType_;

    Float2D<double> b_;
    int               bNX_, bNY_;
    double            boundaryPos_[4];
    RealType          originX_, originY_;
    RealType          bDX_, bDY_;

    Float2D<double> d_;
    int               dNX_, dNY_;
    double            dBoundaryPos_[4];
    RealType          dOriginX_, dOriginY_;
    RealType          dDX_, dDY_;

    bool success_ = true;
  };

} // namespace Scenarios
