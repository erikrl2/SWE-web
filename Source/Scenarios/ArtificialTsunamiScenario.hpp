/**
 * @file ArtificialTsunamiScenario.hpp
 * @brief Defines the ArtificialTsunamiScenario class for simulating artificial tsunami scenarios.
 */

#pragma once

#include "Scenario.hpp"

namespace Scenarios {

  /**
   * @class ArtificialTsunamiScenario
   * @brief A scenario class representing an artificial tsunami simulation.
   */
  class ArtificialTsunamiScenario: public Scenario {
  public:
    /**
     * @brief Constructs an ArtificialTsunamiScenario.
     * @param endSimulationTime The time at which the simulation ends.
     * @param boundaryType The type of boundary conditions for the scenario.
     */
    ArtificialTsunamiScenario(double endSimulationTime, BoundaryType boundaryType);

    /**
     * @brief Default destructor.
     */
    ~ArtificialTsunamiScenario() override = default;

    /**
     * @brief Gets the initial water height at a specific point.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return The initial water height at (x, y).
     */
    RealType getWaterHeight(RealType x, RealType y) const override;

    /**
     * @brief Gets the bathymetry (terrain height) at a specific point.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return The bathymetry at (x, y).
     */
    RealType getBathymetry(RealType x, RealType y) const override;

    /**
     * @brief Gets the boundary type for the scenario.
     * @param edge The boundary edge.
     * @return The boundary type for the specified edge.
     */
    BoundaryType getBoundaryType([[maybe_unused]] BoundaryEdge edge) const override { return boundaryType_; }

    /**
     * @brief Gets the position of a boundary edge.
     * @param edge The boundary edge.
     * @return The position of the specified boundary edge.
     */
    RealType getBoundaryPos(BoundaryEdge edge) const override;

    /**
     * @brief Gets the end simulation time.
     * @return The end simulation time.
     */
    double getEndSimulationTime() const override { return endSimulationTime_; }

  private:
    double       endSimulationTime_; ///< The time at which the simulation ends.
    BoundaryType boundaryType_;      ///< The boundary condition type for the scenario.
  };

} // namespace Scenarios
