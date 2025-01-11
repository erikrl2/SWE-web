#pragma once
#include "Scenario.hpp"
#include "../Types/Float2D.hpp"

#include <string>

namespace Scenarios {
class RealisticScenario : public Scenario {
public:
    RealisticScenario(const std::string& bathymetryFile, 
                      const std::string& displacementFile, 
                      BoundaryType boundaryType);
    
    virtual RealType getBathymetryBeforeDisplacement(RealType x, RealType y) const override;
    virtual RealType getDisplacement(RealType x, RealType y) const override;
    virtual RealType getBoundaryPos(BoundaryEdge edge) const override;
    bool success() const { return success_; }

private:
    struct FileHeader {
        uint32_t nX;
        uint32_t nY;
        double originX;
        double originY;
        double dx;
        double dy;
    };

    bool loadBinaryData(const std::string& filename, 
                       FileHeader& header,
                       Float2D<RealType>& data);

    BoundaryType boundaryType_;
    
    // Bathymetry data
    FileHeader bHeader_;
    Float2D<RealType> bathymetry_;
    
    // Displacement data
    FileHeader dHeader_;
    Float2D<RealType> displacement_;
    
    bool success_ = true;
};
}