/**
 * @file DimensionalSplitting.hpp
 * @brief Implementation of the dimensional splitting scheme for 2D shallow water equations
 */

#pragma once

#include "Blocks/Block.hpp"
#include "Solvers/Fwave.hpp"
#include "Types/Float2D.hpp"

namespace Blocks {

  /**
   * @brief Class implementing the dimensional splitting scheme for shallow water equations
   *
   * This class solves the 2D shallow water equations by splitting them into 1D problems
   * and solving them sequentially using the F-wave solver.
   */
  class DimensionalSplittingBlock: public Block {
  public:
    /**
     * @brief Construct a new Dimensional Splitting solver
     * @param nx Number of cells in x-direction
     * @param ny Number of cells in y-direction
     * @param dx Cell size in x-direction
     * @param dy Cell size in y-direction
     */
    DimensionalSplittingBlock(int nx, int ny, RealType dx, RealType dy);

    DimensionalSplittingBlock(const DimensionalSplittingBlock&) = delete;

    /**
     * @brief Compute numerical fluxes using the F-wave solver
     */
    void computeNumericalFluxes() override;

    /**
     * @brief Update the cell values using computed net updates
     * @param dt Time step size
     */
    void updateUnknowns(RealType dt) override;

    bool hasError() override;

  private:
    /** @brief Net updates for water height (left-going waves) */
    Float2D<RealType> hNetUpdatesLeft_;
    /** @brief Net updates for water height (right-going waves) */
    Float2D<RealType> hNetUpdatesRight_;
    /** @brief Net updates for momentum in x/y-direction (left/down-going waves) */
    Float2D<RealType> huNetUpdatesLeft_;
    /** @brief Net updates for momentum in x/y-direction (right/up-going waves) */
    Float2D<RealType> huNetUpdatesRight_;

    /** @brief F-wave solver instance */
    Solvers::Fwave solver_;
  };

} // namespace Blocks
