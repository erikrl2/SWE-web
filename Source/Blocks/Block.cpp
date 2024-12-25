/**
 * @file
 * This file is part of SWE.
 *
 * @author Michael Bader, Kaveh Rahnema, Tobias Schnabel
 * @author Sebastian Rettenberger (rettenbs AT in.tum.de,
 * http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger,_M.Sc.)
 *
 * @section LICENSE
 *
 * SWE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SWE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SWE.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Block.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>

static constexpr RealType GRAVITY = 9.81f;

Blocks::Block::Block(int nx, int ny, RealType dx, RealType dy):
  nx_(nx),
  ny_(ny),
  dx_(dx),
  dy_(dy),
  h_(ny + 2, nx + 2),
  hu_(ny + 2, nx + 2),
  hv_(ny + 2, nx + 2),
  b_(ny + 2, nx + 2),
  maxTimeStep_(0),
  offsetX_(0),
  offsetY_(0) {

  for (int i = 0; i < 4; i++) {
    boundary_[i] = BoundaryType::Count; // (invalid)
  }
}

void Blocks::Block::initialiseScenario(RealType offsetX, RealType offsetY, const Scenarios::Scenario& scenario) {
  offsetX_ = offsetX;
  offsetY_ = offsetY;

  // Initialize water height and discharge
  for (int j = 1; j <= ny_; j++) {
    for (int i = 1; i <= nx_; i++) {
      RealType x = offsetX + (i - RealType(0.5)) * dx_;
      RealType y = offsetY + (j - RealType(0.5)) * dy_;
      h_[j][i]   = scenario.getWaterHeight(x, y);
      hu_[j][i]  = scenario.getMomentumU(x, y);
      hv_[j][i]  = scenario.getMomentumV(x, y);
    }
  }

  // Initialize bathymetry
  for (int j = 0; j <= ny_ + 1; j++) {
    for (int i = 0; i <= nx_ + 1; i++) {
      b_[j][i] = scenario.getBathymetry(offsetX + (i - RealType(0.5)) * dx_, offsetY + (j - RealType(0.5f)) * dy_);
    }
  }

  // Obtain boundary conditions for all four edges from scenario
  setBoundaryType(BoundaryEdge::Left, scenario.getBoundaryType(BoundaryEdge::Left));
  setBoundaryType(BoundaryEdge::Right, scenario.getBoundaryType(BoundaryEdge::Right));
  setBoundaryType(BoundaryEdge::Bottom, scenario.getBoundaryType(BoundaryEdge::Bottom));
  setBoundaryType(BoundaryEdge::Top, scenario.getBoundaryType(BoundaryEdge::Top));

  // Perform update after external write to variables
  synchAfterWrite();
}

void Blocks::Block::setWaterHeight(RealType (*h)(RealType, RealType)) {
  for (int j = 1; j <= ny_; j++) {
    for (int i = 1; i <= nx_; i++) {
      h_[j][i] = h(offsetX_ + (i - RealType(0.5)) * dx_, offsetY_ + (j - RealType(0.5)) * dy_);
    }
  }

  synchWaterHeightAfterWrite();
}

void Blocks::Block::setDischarge(RealType (*u)(RealType, RealType), RealType (*v)(RealType, RealType)) {
  for (int j = 1; j <= ny_; j++) {
    for (int i = 1; i <= nx_; i++) {
      RealType x = offsetX_ + (i - RealType(0.5)) * dx_;
      RealType y = offsetY_ + (j - RealType(0.5)) * dy_;
      hu_[j][i]  = u(x, y) * h_[j][i];
      hv_[j][i]  = v(x, y) * h_[j][i];
    };
  }

  synchDischargeAfterWrite();
}

void Blocks::Block::setBathymetry(RealType b) {
  for (int j = 0; j <= ny_ + 1; j++) {
    for (int i = 0; i <= nx_ + 1; i++) {
      b_[j][i] = b;
    }
  }

  synchBathymetryAfterWrite();
}

void Blocks::Block::setBathymetry(RealType (*b)(RealType, RealType)) {
  for (int j = 0; j <= ny_ + 1; j++) {
    for (int i = 0; i <= nx_ + 1; i++) {
      b_[j][i] = b(offsetX_ + (i - RealType(0.5)) * dx_, offsetY_ + (j - RealType(0.5)) * dy_);
    }
  }

  synchBathymetryAfterWrite();
}

const Float2D<RealType>& Blocks::Block::getWaterHeight() {
  synchWaterHeightBeforeRead();
  return h_;
}

const Float2D<RealType>& Blocks::Block::getDischargeHu() {
  synchDischargeBeforeRead();
  return hu_;
}

const Float2D<RealType>& Blocks::Block::getDischargeHv() {
  synchDischargeBeforeRead();
  return hv_;
}

const Float2D<RealType>& Blocks::Block::getBathymetry() {
  synchBathymetryBeforeRead();
  return b_;
}

void Blocks::Block::simulateTimeStep(RealType dt) {
  computeNumericalFluxes();
  updateUnknowns(dt);
}

RealType Blocks::Block::simulate(RealType tStart, RealType tEnd) {
  RealType t = tStart;
  do {
    setGhostLayer();

    computeNumericalFluxes();
    updateUnknowns(maxTimeStep_);
    t += maxTimeStep_;

    std::cout << "Simulation at time " << t << std::endl << std::flush;
  } while (t < tEnd);

  return t;
}

void Blocks::Block::setBoundaryType(BoundaryEdge edge, BoundaryType boundaryType) {
  boundary_[edge] = boundaryType;

  if (boundaryType == BoundaryType::Outflow || boundaryType == BoundaryType::Wall) {
    // One of the boundary was changed to BoundaryType::Outflow or BoundaryType::Wall
    // -> Update the bathymetry for this boundary
    setBoundaryBathymetry();
  }
}

void Blocks::Block::setBoundaryBathymetry() {
  // Set bathymetry values in the ghost layer, if necessary
  if (boundary_[BoundaryEdge::Left] == BoundaryType::Outflow || boundary_[BoundaryEdge::Left] == BoundaryType::Wall) {
    for (int j = 0; j <= ny_ + 1; j++) {
      b_[j][0] = b_[j][1];
    }
  }
  if (boundary_[BoundaryEdge::Right] == BoundaryType::Outflow || boundary_[BoundaryEdge::Right] == BoundaryType::Wall) {
    for (int j = 0; j <= ny_ + 1; j++) {
      b_[j][nx_ + 1] = b_[j][nx_];
    }
  }
  if (boundary_[BoundaryEdge::Bottom] == BoundaryType::Outflow || boundary_[BoundaryEdge::Bottom] == BoundaryType::Wall) {
    std::memcpy(b_[0], b_[1], sizeof(RealType) * (nx_ + 2));
  }
  if (boundary_[BoundaryEdge::Top] == BoundaryType::Outflow || boundary_[BoundaryEdge::Top] == BoundaryType::Wall) {
    std::memcpy(b_[ny_ + 1], b_[ny_], sizeof(RealType) * (nx_ + 2));
  }

  // Set corner values
  b_[0][0]             = b_[1][1];
  b_[0][nx_ + 1]       = b_[1][nx_];
  b_[ny_ + 1][0]       = b_[ny_][1];
  b_[ny_ + 1][nx_ + 1] = b_[ny_][nx_];

  // Synchronize after an external update of the bathymetry
  synchBathymetryAfterWrite();
}

void Blocks::Block::setGhostLayer() {
  // std::cout << "Set simple boundary conditions " << std::endl << std::flush;
  //  Call to virtual function to set ghost layer values
  setBoundaryConditions();

  //  Synchronize the ghost layers with accelerator memory
  synchGhostLayerAfterWrite();
}

void Blocks::Block::computeMaxTimeStep(const RealType dryTol, const RealType cfl) {
  // Initialize the maximum wave speed
  RealType maximumWaveSpeed = RealType(0.0);

  // Compute the maximum wave speed within the grid
  for (int j = 1; j <= ny_; j++) {
    for (int i = 1; i <= nx_; i++) {
      if (h_[j][i] > dryTol) {
        RealType momentum = std::max(std::abs(hu_[j][i]), std::abs(hv_[j][i]));

        RealType particleVelocity = momentum / h_[j][i];

        // Approximate the wave speed
        RealType waveSpeed = particleVelocity + std::sqrt(GRAVITY * h_[j][i]);

        maximumWaveSpeed = std::max(maximumWaveSpeed, waveSpeed);
      }
    }
  }

  RealType minimumCellLength = std::min(dx_, dy_);

  // Set the maximum time step variable
  maxTimeStep_ = minimumCellLength / maximumWaveSpeed;

  // Apply the CFL condition
  maxTimeStep_ *= cfl;
}

RealType Blocks::Block::getMaxTimeStep() const { return maxTimeStep_; }

void Blocks::Block::synchAfterWrite() {
  synchWaterHeightAfterWrite();
  synchDischargeAfterWrite();
  synchBathymetryAfterWrite();
}

void Blocks::Block::synchWaterHeightAfterWrite() {}

void Blocks::Block::synchDischargeAfterWrite() {}

void Blocks::Block::synchBathymetryAfterWrite() {}

void Blocks::Block::synchGhostLayerAfterWrite() {}

void Blocks::Block::synchBeforeRead() {
  synchWaterHeightBeforeRead();
  synchDischargeBeforeRead();
  synchBathymetryBeforeRead();
}

void Blocks::Block::synchWaterHeightBeforeRead() {}

void Blocks::Block::synchDischargeBeforeRead() {}

void Blocks::Block::synchBathymetryBeforeRead() {}

void Blocks::Block::synchCopyLayerBeforeRead() {}

void Blocks::Block::setBoundaryConditions() {
  // BoundaryType::Passive conditions need to be set by the component using Blocks::Block

  // Left boundary
  switch (boundary_[BoundaryEdge::Left]) {
  case BoundaryType::Wall: {
    for (int j = 1; j <= ny_; j++) {
      h_[j][0]  = h_[j][1];
      hu_[j][0] = -hu_[j][1];
      hv_[j][0] = hv_[j][1];
    };
    break;
  }
  case BoundaryType::Outflow: {
    for (int j = 1; j <= ny_; j++) {
      h_[j][0]  = h_[j][1];
      hu_[j][0] = hu_[j][1];
      hv_[j][0] = hv_[j][1];
    };
    break;
  }
  default:
    assert(false);
    break;
  };

  // Right boundary
  switch (boundary_[BoundaryEdge::Right]) {
  case BoundaryType::Wall: {
    for (int j = 1; j <= ny_; j++) {
      h_[j][nx_ + 1]  = h_[j][nx_];
      hu_[j][nx_ + 1] = -hu_[j][nx_];
      hv_[j][nx_ + 1] = hv_[j][nx_];
    };
    break;
  }
  case BoundaryType::Outflow: {
    for (int j = 1; j <= ny_; j++) {
      h_[j][nx_ + 1]  = h_[j][nx_];
      hu_[j][nx_ + 1] = hu_[j][nx_];
      hv_[j][nx_ + 1] = hv_[j][nx_];
    };
    break;
  }
  default:
    assert(false);
    break;
  };

  // Bottom boundary
  switch (boundary_[BoundaryEdge::Bottom]) {
  case BoundaryType::Wall: {
    for (int i = 1; i <= nx_; i++) {
      h_[0][i]  = h_[1][i];
      hu_[0][i] = hu_[1][i];
      hv_[0][i] = -hv_[1][i];
    };
    break;
  }
  case BoundaryType::Outflow: {
    for (int i = 1; i <= nx_; i++) {
      h_[0][i]  = h_[1][i];
      hu_[0][i] = hu_[1][i];
      hv_[0][i] = hv_[1][i];
    };
    break;
  }
  default:
    assert(false);
    break;
  };

  // Top boundary
  switch (boundary_[BoundaryEdge::Top]) {
  case BoundaryType::Wall: {
    for (int i = 1; i <= nx_; i++) {
      h_[ny_ + 1][i]  = h_[ny_][i];
      hu_[ny_ + 1][i] = hu_[ny_][i];
      hv_[ny_ + 1][i] = -hv_[ny_][i];
    };
    break;
  }
  case BoundaryType::Outflow: {
    for (int i = 1; i <= nx_; i++) {
      h_[ny_ + 1][i]  = h_[ny_][i];
      hu_[ny_ + 1][i] = hu_[ny_][i];
      hv_[ny_ + 1][i] = hv_[ny_][i];
    };
    break;
  }
  default:
    assert(false);
    break;
  };

  /*
   * Set values in corner ghost cells. Required for dimensional splitting and visualization.
   *   The quantities in the corner ghost cells are chosen to generate a zero Riemann solutions
   *   (steady state) with the neighboring cells. For the lower left corner (0,0) using
   *   the values of (1,1) generates a steady state (zero) Riemann problem for (0,0) - (0,1) and
   *   (0,0) - (1,0) for both outflow and reflecting boundary conditions.
   *
   *   Remark: Unsplit methods don't need corner values.
   *
   * Sketch (reflecting boundary conditions, lower left corner):
   * <pre>
   *                  **************************
   *                  *  _    _    *  _    _   *
   *  Ghost           * |  h   |   * |  h   |  *
   *  cell    ------> * | -hu  |   * |  hu  |  * <------ Cell (1,1) inside the domain
   *  (0,1)           * |_ hv _|   * |_ hv _|  *
   *                  *            *           *
   *                  **************************
   *                  *  _    _    *  _    _   *
   *   Corner Ghost   * |  h   |   * |  h   |  *
   *   cell   ------> * |  hu  |   * |  hu  |  * <----- Ghost cell (1,0)
   *   (0,0)          * |_ hv _|   * |_-hv _|  *
   *                  *            *           *
   *                  **************************
   * </pre>
   */
  h_[0][0]  = h_[1][1];
  hu_[0][0] = hu_[1][1];
  hv_[0][0] = hv_[1][1];

  h_[ny_ + 1][0]  = h_[ny_][1];
  hu_[ny_ + 1][0] = hu_[ny_][1];
  hv_[ny_ + 1][0] = hv_[ny_][1];

  h_[0][nx_ + 1]  = h_[1][nx_];
  hu_[0][nx_ + 1] = hu_[1][nx_];
  hv_[0][nx_ + 1] = hv_[1][nx_];

  h_[ny_ + 1][nx_ + 1]  = h_[ny_][nx_];
  hu_[ny_ + 1][nx_ + 1] = hu_[ny_][nx_];
  hv_[ny_ + 1][nx_ + 1] = hv_[ny_][nx_];
}

int Blocks::Block::getNx() const { return nx_; }

int Blocks::Block::getNy() const { return ny_; }

RealType Blocks::Block::getDx() const { return dx_; }

RealType Blocks::Block::getDy() const { return dy_; }

RealType Blocks::Block::getOffsetX() const { return offsetX_; }

RealType Blocks::Block::getOffsetY() const { return offsetY_; }
