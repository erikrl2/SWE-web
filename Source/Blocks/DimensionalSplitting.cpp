#include "DimensionalSplitting.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace Blocks {

  DimensionalSplittingBlock::DimensionalSplittingBlock(int nx, int ny, RealType dx, RealType dy):
    Block(nx, ny, dx, dy),
    hNetUpdatesLeft_(ny + 2, nx + 1),
    hNetUpdatesRight_(ny + 2, nx + 1),
    huNetUpdatesLeft_(ny + 2, nx + 1),
    huNetUpdatesRight_(ny + 2, nx + 1) {}

  void DimensionalSplittingBlock::computeNumericalFluxes() {
    // X-Sweep:

    RealType maxWaveSpeedX = RealType(0.0);

// Loop over all vertical edges
#ifdef ENABLE_OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int y = 0; y < ny_ + 2; y++) {
      for (int x = 1; x < nx_ + 2; x++) {
        RealType maxEdgeSpeedX = RealType(0.0);

        // Compute net updates
        solver_.computeNetUpdates(
          h_[y][x - 1],
          h_[y][x],
          hu_[y][x - 1],
          hu_[y][x],
          b_[y][x - 1],
          b_[y][x],
          hNetUpdatesLeft_[y][x - 1],
          hNetUpdatesRight_[y][x - 1],
          huNetUpdatesLeft_[y][x - 1],
          huNetUpdatesRight_[y][x - 1],
          maxEdgeSpeedX
        );

        // Update maxWaveSpeed
        if (maxEdgeSpeedX > maxWaveSpeedX) {
          maxWaveSpeedX = maxEdgeSpeedX;
        }
      }
    }

    assert(maxWaveSpeedX > RealType(0.0));

    // Compute CFL condition
    maxTimeStep_ = dx_ / maxWaveSpeedX * RealType(0.4);
  }

  void DimensionalSplittingBlock::updateUnknowns(RealType dt) {
// Loop over all inner cells
#ifdef ENABLE_OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int y = 0; y < ny_ + 2; y++) {
      for (int x = 1; x < nx_ + 1; x++) {
        h_[y][x] -= dt / dx_ * (hNetUpdatesRight_[y][x - 1] + hNetUpdatesLeft_[y][x]);
        hu_[y][x] -= dt / dx_ * (huNetUpdatesRight_[y][x - 1] + huNetUpdatesLeft_[y][x]);
      }
    }

    // Y-Sweep:

    RealType maxWaveSpeedY = RealType(0.0);

// Loop over horizontal edges
#ifdef ENABLE_OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int y = 1; y < ny_ + 2; y++) {
      for (int x = 1; x < nx_ + 1; x++) {
        RealType maxEdgeSpeedY = RealType(0.0);

        // Compute net updates
        solver_.computeNetUpdates(
          h_[y - 1][x],
          h_[y][x],
          hv_[y - 1][x],
          hv_[y][x],
          b_[y - 1][x],
          b_[y][x],
          hNetUpdatesLeft_[y - 1][x],
          hNetUpdatesRight_[y - 1][x],
          huNetUpdatesLeft_[y - 1][x],  // reuse huNetUpdatesLeft_ as hvNetUpdatesLeft_
          huNetUpdatesRight_[y - 1][x], // reuse huNetUpdatesRight_ as hvNetUpdatesRight_
          maxEdgeSpeedY
        );

        // Update maxWaveSpeed
        if (maxEdgeSpeedY > maxWaveSpeedY) {
          maxWaveSpeedY = maxEdgeSpeedY;
        }
      }
    }

#ifndef NDEBUG
    // assert(dt < RealType(0.5) * dy_ / maxWaveSpeedY);
    // Use exception instead of assert to be able to catch it in unit tests
    if (dt >= RealType(0.5) * dy_ / maxWaveSpeedY) {
      throw std::runtime_error("CFL condition not satisfied in y-sweep!");
    }
#endif

// Loop over all inner cells
#ifdef ENABLE_OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int y = 1; y < ny_ + 1; y++) {
      for (int x = 1; x < nx_ + 1; x++) {
        h_[y][x] -= dt / dy_ * (hNetUpdatesRight_[y - 1][x] + hNetUpdatesLeft_[y][x]);
        hv_[y][x] -= dt / dy_ * (huNetUpdatesRight_[y - 1][x] + huNetUpdatesLeft_[y][x]);
      }
    }
  }

} // namespace Blocks
