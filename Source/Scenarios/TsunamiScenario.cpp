#ifdef ENABLE_NETCDF
#include "TsunamiScenario.hpp"

#include <cassert>
#include <cmath>
#include <netcdf>

Scenarios::TsunamiScenario::TsunamiScenario(const std::string& bathymetryFile, const std::string& displacementFile, BoundaryType boundaryType, int nX, int nY):
  boundaryType_(boundaryType),
  nX_(nX),
  nY_(nY) {
  // Bathymetry file
  try {
    std::cout << "Reading bathymetry file " << bathymetryFile << std::endl;

    netCDF::NcFile dataFile(bathymetryFile, netCDF::NcFile::read);

    bNX_ = dataFile.getDim("x").getSize();
    bNY_ = dataFile.getDim("y").getSize();

    assert(bNX_ >= 2 && bNY_ >= 2);

    auto xVar = dataFile.getVar("x");
    auto yVar = dataFile.getVar("y");
    auto bVar = dataFile.getVar("z");

    // Sample two coordinate values to determine cell size
    RealType sampleX[2];
    RealType sampleY[2];

    xVar.getVar({0}, {2}, sampleX);
    yVar.getVar({0}, {2}, sampleY);

    bDX_ = sampleX[1] - sampleX[0];
    bDY_ = sampleY[1] - sampleY[0];

    // Determine bathymetry domain boundaries
    boundaryPos_[0] = sampleX[0] - bDX_ / 2.0;
    boundaryPos_[1] = boundaryPos_[0] + bDX_ * bNX_;
    boundaryPos_[2] = sampleY[0] - bDY_ / 2.0;
    boundaryPos_[3] = boundaryPos_[2] + bDY_ * bNY_;

    originX_ = boundaryPos_[0];
    originY_ = boundaryPos_[2];

    // Determine cell size later used by simulation
    dX_ = (boundaryPos_[1] - boundaryPos_[0]) / nX_;
    dY_ = (boundaryPos_[3] - boundaryPos_[2]) / nY_;

    b_ = Float2D<RealType>(bNY_, bNX_);

    bVar.getVar(b_.getData());

  } catch (netCDF::exceptions::NcException& e) {
    // std::cerr << e.what() << std::endl;
    success_ = false;
    return;
  }

  // Displacement file
  try {
    std::cout << "Reading displacement file " << displacementFile << std::endl;

    netCDF::NcFile dataFile(displacementFile, netCDF::NcFile::read);

    dNX_ = dataFile.getDim("x").getSize();
    dNY_ = dataFile.getDim("y").getSize();

    assert(dNX_ >= 2 && dNY_ >= 2);

    auto xVar = dataFile.getVar("x");
    auto yVar = dataFile.getVar("y");
    auto dVar = dataFile.getVar("z");

    // Sample two coordinate values to determine cell size
    RealType sampleX[2];
    RealType sampleY[2];

    xVar.getVar({0}, {2}, sampleX);
    yVar.getVar({0}, {2}, sampleY);

    dDX_ = sampleX[1] - sampleX[0];
    dDY_ = sampleY[1] - sampleY[0];

    // Determine displacement domain boundaries
    dBoundaryPos_[0] = sampleX[0] - dDX_ / 2.0;
    dBoundaryPos_[1] = dBoundaryPos_[0] + dDX_ * dNX_;
    dBoundaryPos_[2] = sampleY[0] - dDY_ / 2.0;
    dBoundaryPos_[3] = dBoundaryPos_[2] + dDY_ * dNY_;

    // Displacement should be located within the domain of the bathymetry
    assert(dBoundaryPos_[0] >= boundaryPos_[0]);
    assert(dBoundaryPos_[1] <= boundaryPos_[1]);
    assert(dBoundaryPos_[2] >= boundaryPos_[2]);
    assert(dBoundaryPos_[3] <= boundaryPos_[3]);

    dOriginX_ = dBoundaryPos_[0];
    dOriginY_ = dBoundaryPos_[2];

    d_ = Float2D<RealType>(dNY_, dNX_);

    dVar.getVar(d_.getData());

  } catch (netCDF::exceptions::NcException& e) {
    // std::cerr << e.what() << std::endl;
    success_ = false;
    return;
  }
}

RealType Scenarios::TsunamiScenario::getBathymetryBeforeDisplacement(RealType x, RealType y) const {
  if (x < boundaryPos_[0] || x > boundaryPos_[1] || y < boundaryPos_[2] || y > boundaryPos_[3]) {
    return RealType(0.0); // Will be later replaced by a boundary condition
  }

  // Determine indices
  int i = std::round((x - originX_) / bDX_ - 0.5);
  int j = std::round((y - originY_) / bDY_ - 0.5);

  assert(i >= 0 && i < bNX_);
  assert(j >= 0 && j < bNY_);

  RealType b = b_[j][i];

  // Clamp the bathymetry to a minimum of 20m
  if (std::abs(b) < 20.0) {
    b = RealType(20.0 * std::copysign(1.0, b));
  }

  return b;
}

RealType Scenarios::TsunamiScenario::getDisplacement(RealType x, RealType y) const {
  if (x < dBoundaryPos_[0] || x > dBoundaryPos_[1] || y < dBoundaryPos_[2] || y > dBoundaryPos_[3]) {
    return RealType(0.0); // No displacement
  }

  // Determine indices
  int i = std::round((x - dOriginX_) / dDX_ - 0.5);
  int j = std::round((y - dOriginY_) / dDY_ - 0.5);

  // Handle edge cases
  if (x == dBoundaryPos_[0])
    i = 0;
  if (x == dBoundaryPos_[1])
    i = dNX_ - 1;
  if (y == dBoundaryPos_[2])
    j = 0;
  if (y == dBoundaryPos_[3])
    j = dNY_ - 1;

  assert(i >= 0 && i < dNX_);
  assert(j >= 0 && j < dNY_);

  return d_[j][i];
}
#endif
