#pragma once
/**
 * @file Fwave.hpp
 * @brief Header file for the F-Wave Solver class.
 *
 * This file defines the `fwave` class, which implements the f-wave method for solving
 * shallow water equations. The class provides a method for computing the net updates
 * of water state variables based on left and right states.
 */

#include "Types/RealType.hpp"

namespace Solvers {

  /**
   * @class Fwave
   * @brief Class for solving shallow water equations using the f-wave method.
   *
   * This class implements methods for calculating net updates of the water state.
   * The F-Wave Solver uses Roe linearization to approximate wave speeds and compute
   * the influence of waves on the left and right states based on their initial conditions.
   */
  class Fwave {
  public:
    /**
     * @brief Computes the net updates for the left and right states using the f-wave method.
     *
     * This method approximates the wave speeds using Roe averages and decomposes the
     * flux differences into contributions of left-going and right-going waves. It then
     * calculates the net updates for these waves, which represent changes in height and
     * momentum due to the propagation of the waves.
     *
     * @param hLeft height on the left side of the edge.
     * @param hRight height on the right side of the edge.
     * @param huLeft momentum on the left side of the edge.
     * @param huRight momentum on the right side of the edge.
     * @param bLeft bathymetry on the left side of the edge.
     * @param bRight bathymetry on the right side of the edge.
     *
     * @param o_hUpdateLeft will be set to: Net-update for the height of the cell on the left side of the edge.
     * @param o_hUpdateRight will be set to: Net-update for the height of the cell on the right side of the edge.
     * @param o_huUpdateLeft will be set to: Net-update for the momentum of the cell on the left side of the edge.
     * @param o_huUpdateRight will be set to: Net-update for the momentum of the cell on the right side of the edge.
     * @param o_maxWaveSpeed will be set to: Maximum (linearized) wave speed -> Should be used in the CFL-condition.
     *
     * @note The bathymetry values (bLeft and bRight) are used to account for the changes in
     * the bottom surface that affect the wave propagation. This method also handles cases
     * where wave speeds might have the same sign (supersonic cases) and adjusts the net
     * updates accordingly.
     */
    void computeNetUpdates(
      RealType  hLeft,
      RealType  hRight,
      RealType  huLeft,
      RealType  huRight,
      RealType  bLeft,
      RealType  bRight,
      RealType& o_hUpdateLeft,
      RealType& o_hUpdateRight,
      RealType& o_huUpdateLeft,
      RealType& o_huUpdateRight,
      RealType& o_maxWaveSpeed
    );

    bool Error = false;
  };

} // namespace Solvers
