#include "RealisticScenario.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

Scenarios::RealisticScenario::RealisticScenario(RealisticScenarioType scenario, BoundaryType boundaryType):
  boundaryType_(boundaryType) {
  std::string bathymetryFile, displacementFile;

  switch (scenario) {
  case RealisticScenarioType::Tohoku:
    bathymetryFile   = "Assets/Data/tohoku_bath.bin";
    displacementFile = "Assets/Data/tohoku_displ.bin";
    break;
  case RealisticScenarioType::Chile:
    bathymetryFile   = "Assets/Data/chile_bath.bin";
    displacementFile = "Assets/Data/chile_displ.bin";
    break;
  default:
    assert(false);
  }

  // Bathymetry file
  try {
    // std::cout << "Reading bathymetry file " << bathymetryFile << std::endl;
    FileHeader header;

    if (!loadBinaryData(bathymetryFile, header, b_)) {
      success_ = false;
      return;
    }

    bNX_ = header.nX;
    bNY_ = header.nY;
    bDX_ = header.dx;
    bDY_ = header.dy;

    assert(bNX_ >= 2 && bNY_ >= 2);

    // Determine bathymetry domain boundaries
    boundaryPos_[0] = header.originX - bDX_ / 2.0;
    boundaryPos_[1] = boundaryPos_[0] + bDX_ * bNX_;
    boundaryPos_[2] = header.originY - bDY_ / 2.0;
    boundaryPos_[3] = boundaryPos_[2] + bDY_ * bNY_;

    originX_ = boundaryPos_[0];
    originY_ = boundaryPos_[2];

  } catch (const std::exception& e) {
    // std::cerr << "Error reading bathymetry file: " << e.what() << std::endl;
    success_ = false;
    return;
  }

  // Displacement file
  try {
    // std::cout << "Reading displacement file " << displacementFile << std::endl;
    FileHeader header;

    if (!loadBinaryData(displacementFile, header, d_)) {
      success_ = false;
      return;
    }

    dNX_ = header.nX;
    dNY_ = header.nY;
    dDX_ = header.dx;
    dDY_ = header.dy;

    assert(dNX_ >= 2 && dNY_ >= 2);

    // Determine displacement domain boundaries
    dBoundaryPos_[0] = header.originX - dDX_ / 2.0;
    dBoundaryPos_[1] = dBoundaryPos_[0] + dDX_ * dNX_;
    dBoundaryPos_[2] = header.originY - dDY_ / 2.0;
    dBoundaryPos_[3] = dBoundaryPos_[2] + dDY_ * dNY_;

    // Displacement should be located within the domain of the bathymetry
    assert(dBoundaryPos_[0] >= boundaryPos_[0]);
    assert(dBoundaryPos_[1] <= boundaryPos_[1]);
    assert(dBoundaryPos_[2] >= boundaryPos_[2]);
    assert(dBoundaryPos_[3] <= boundaryPos_[3]);

    dOriginX_ = dBoundaryPos_[0];
    dOriginY_ = dBoundaryPos_[2];

  } catch (const std::exception& e) {
    // std::cerr << "Error reading displacement file: " << e.what() << std::endl;
    success_ = false;
    return;
  }
}

bool Scenarios::RealisticScenario::loadBinaryData(const std::string& filename, FileHeader& header, Float2D<double>& data) {

  std::ifstream file(filename, std::ios::binary);
  if (!file)
    return false;

  // Read header
  file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
  if (!file)
    return false;

  // Allocate and read data
  data = Float2D<double>(header.nY, header.nX);
  file.read(reinterpret_cast<char*>(data.getData()), header.nX * header.nY * sizeof(double));

  return file.good();
}

RealType Scenarios::RealisticScenario::getBathymetryBeforeDisplacement(RealType x, RealType y) const {
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

RealType Scenarios::RealisticScenario::getDisplacement(RealType x, RealType y) const {
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
