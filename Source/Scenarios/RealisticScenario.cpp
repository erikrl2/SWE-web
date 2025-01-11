#include "RealisticScenario.hpp"
#include <fstream>
#include <cmath>
#include <iostream>

Scenarios::RealisticScenario::RealisticScenario(
    const std::string& bathymetryFile,
    const std::string& displacementFile,
    BoundaryType boundaryType)
    : boundaryType_(boundaryType) {

    success_ = loadBinaryData(bathymetryFile, bHeader_, bathymetry_);
    if (!success_) {
        std::cerr << "Failed to load bathymetry file at "  << std::endl;
        return;
    }

    success_ = loadBinaryData(displacementFile, dHeader_, displacement_);
    if (!success_) {
        std::cerr << "Failed to load displacement file" << std::endl;
        return;
    }
}

bool Scenarios::RealisticScenario::loadBinaryData(
    const std::string& filename,
    FileHeader& header,
    Float2D<RealType>& data) {
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    // Read header
    file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
    
    // Allocate and read data
    data = Float2D<RealType>(header.nY, header.nX);
    file.read(reinterpret_cast<char*>(data.getData()), 
              header.nX * header.nY * sizeof(RealType));
    
    return file.good();
}

RealType Scenarios::RealisticScenario::getBathymetryBeforeDisplacement(
    RealType x, RealType y) const {
    
    // Convert coordinates to indices
    int i = std::round((x - bHeader_.originX) / bHeader_.dx);
    int j = std::round((y - bHeader_.originY) / bHeader_.dy);
    
    // Check bounds
    if (i < 0 || i >= bHeader_.nX || j < 0 || j >= bHeader_.nY) {
        return RealType(0.0);
    }
    
    RealType b = bathymetry_[j][i];
    
    // Clamp bathymetry to minimum of 20m (same as original)
    if (std::abs(b) < 20.0) {
        b = RealType(20.0 * std::copysign(1.0, b));
    }
    
    return b;
}

RealType Scenarios::RealisticScenario::getDisplacement(
    RealType x, RealType y) const {
    
    // Convert coordinates to indices
    int i = std::round((x - dHeader_.originX) / dHeader_.dx);
    int j = std::round((y - dHeader_.originY) / dHeader_.dy);
    
    // Check bounds
    if (i < 0 || i >= dHeader_.nX || j < 0 || j >= dHeader_.nY) {
        return RealType(0.0);
    }
    
    return displacement_[j][i];
}

RealType Scenarios::RealisticScenario::getBoundaryPos(BoundaryEdge edge) const {
    switch (edge) {
        case BoundaryEdge::Left:
            return bHeader_.originX;
        case BoundaryEdge::Right:
            return bHeader_.originX + bHeader_.dx * bHeader_.nX;
        case BoundaryEdge::Bottom:
            return bHeader_.originY;
        case BoundaryEdge::Top:
            return bHeader_.originY + bHeader_.dy * bHeader_.nY;
    }
    return 0.0;
}