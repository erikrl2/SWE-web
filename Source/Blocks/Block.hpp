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

#pragma once

#include "Scenarios/Scenario.hpp"
#include "Types/BoundaryEdge.hpp"
#include "Types/BoundaryType.hpp"
#include "Types/Float2D.hpp"
#include "Types/RealType.hpp"

namespace Blocks {

  class Block {
  protected:
    // Grid size: number of cells (incl. ghost layer in x and y direction):
    int nx_; ///< Size of Cartesian arrays in x-direction
    int ny_; ///< Size of Cartesian arrays in y-direction
    // Mesh size dx and dy:
    RealType dx_; ///< Mesh size of the Cartesian grid in x-direction
    RealType dy_; ///< Mesh size of the Cartesian grid in y-direction

    // Define arrays for unknowns:
    // h (water level) and u, v (velocity in x and y direction)
    // hd, ud, and vd are respective CUDA arrays on GPU
    Float2D<RealType> h_;  ///< Array that holds the water height for each element
    Float2D<RealType> hu_; ///< Array that holds the x-component of the momentum for each element (water height h
                           ///< multiplied by velocity in x-direction)
    Float2D<RealType> hv_; ///< Array that holds the y-component of the momentum for each element (water height h
                           ///< multiplied by velocity in y-direction)
    Float2D<RealType> b_;  ///< Array that holds the bathymetry data (sea floor elevation) for each element

    /// Type of boundary conditions at Left, Right, Top, and Bottom boundary
    BoundaryType boundary_[4];

    /// Maximum time step allowed to ensure stability of the method
    /**
     * maxTimeStep_ can be updated as part of the methods computeNumericalFluxes
     * and updateUnknowns (depending on the numerical method)
     */
    RealType maxTimeStep_;

    // Offset of current block
    RealType offsetX_; ///< x-coordinate of the origin (left-bottom corner) of the Cartesian grid
    RealType offsetY_; ///< y-coordinate of the origin (left-bottom corner) of the Cartesian grid

    /**
     * Constructor: allocate variables for simulation
     *
     * unknowns h (water height), hu,hv (discharge in x- and y-direction),
     * and b (bathymetry) are defined on grid indices [0,..,nx+1]*[0,..,ny+1]
     * -> computational domain is [1,..,nx]*[1,..,ny]
     * -> plus ghost cell layer
     *
     * The constructor is protected: no instances of Blocks::Block can be
     * generated.
     *
     */
    Block(int nx, int ny, RealType dx, RealType dy);

    /**
     * Sets the bathymetry on BoundaryType::Outflow or BoundaryType::Wall.
     * Should be called very time a boundary is changed to a BoundaryType::Outflow or
     * BoundaryType::Wall <b>or</b> the bathymetry changes.
     */
    void setBoundaryBathymetry();

    /// Sets boundary conditions in ghost layers (set boundary conditions)
    /**
     * Sets the values of all ghost cells depending on the specifed
     * boundary conditions
     * - set boundary conditions for types BoundaryType::Wall and BoundaryType::Outflow
     * - derived classes need to transfer ghost layers
     */
    virtual void setBoundaryConditions();

  public:
    /**
     * Destructor: de-allocate all variables
     */
    virtual ~Block() = default;

    /// Initialises unknowns to a specific scenario
    /**
     * Initialises the unknowns and bathymetry in all grid cells according to the given Scenarios::Scenario.
     *
     * In the case of multiple Blocks::Block at this point, it is not clear how the boundary conditions
     * should be set. This is because an isolated Blocks::Block doesn't have any information about the grid.
     * Therefore the calling routine, which has the information about multiple blocks, has to take care about setting
     * the right boundary conditions.
     *
     * @param scenario Scenarios::Scenario, which is used during the setup.
     * @param useMultipleBlocks Are there multiple blocks?
     */
    void initialiseScenario(RealType offsetX, RealType offsetY, const Scenarios::Scenario& scenario);

    /// Sets the water height according to a given function
    /**
     * Sets water height h in all interior grid cells (i.e. except ghost layer)
     * to values specified by parameter function h.
     */
    void setWaterHeight(RealType (*h)(RealType, RealType));

    /// Sets the momentum/discharge according to the provided functions
    /**
     * Sets discharge in all interior grid cells (i.e. except ghost layer)
     * to values specified by parameter functions.
     * Note: unknowns hu and hv represent momentum, while parameters u and v are velocities!
     */
    void setDischarge(RealType (*u)(RealType, RealType), RealType (*v)(RealType, RealType));

    /// Sets the bathymetry to a uniform value
    /**
     * Sets Bathymetry b in all grid cells (incl. ghost/boundary layers)
     * to a uniform value;
     * bathymetry source terms are re-computed.
     */
    void setBathymetry(RealType b);

    /// Sets the bathymetry according to a given function
    /**
     * Sets Bathymetry b in all grid cells (incl. ghost/boundary layers)
     * using the specified bathymetry function;
     * bathymetry source terms are re-computed.
     */
    void setBathymetry(RealType (*b)(RealType, RealType));

    // Read access to arrays of unknowns
    const Float2D<RealType>& getWaterHeight() const;
    const Float2D<RealType>& getDischargeHu() const;
    const Float2D<RealType>& getDischargeHv() const;
    const Float2D<RealType>& getBathymetry() const;

    /**
     * Sets the boundary type for specific block boundary.
     *
     * @param edge Location of the edge relative to the Blocks::Block.
     * @param boundaryType Type of the boundary condition.
     * @param inflow Pointer to an Blocks::Block1D, which specifies the inflow (should be nullptr for
     * BoundaryType::Wall or BoundaryType::Outflow).
     */
    void setBoundaryType(BoundaryEdge edge, BoundaryType boundaryType);

    /**
     * Sets the values of all ghost cells depending on the specifed
     * boundary conditions;
     * if the ghost layer replicates the variables of a remote Blocks::Block,
     * the values are copied.
     */
    void setGhostLayer();

    /**
     * Computes the largest allowed time step for the current grid block
     * (reference implementation) depending on the current values of
     * variables h, hu, and hv, and store this time step size in member
     * variable maxTimestep.
     *
     * @param dryTol Dry tolerance (dry cells do not affect the time step).
     * @param cfl CFL number of the used method.
     */
    void computeMaxTimeStep(const RealType dryTol = 0.1f, const RealType cfl = 0.4f);

    /// Returns maximum size of the time step to ensure stability of the method
    RealType getMaxTimeStep() const;

    /// Executes a single time step (with fixed time step size) of the simulation
    virtual void simulateTimeStep(RealType dt);

    /// Performs the simulation starting with simulation time tStart, until simulation time tEnd is reached
    /**
     * Implements the main simulation loop between two checkpoints;
     * Note: this implementation can only be used, if you only use a single Blocks::Block
     *       and only apply simple boundary conditions!
     *       In particular, Blocks::Block::simulate can not trigger calls to exchange values
     *       of copy and ghost layers between blocks!
     * @param tStart Time where the simulation is started
     * @param tEnd Time of the next checkpoint
     * @return Actual end time reached
     */
    virtual RealType simulate(RealType tStart, RealType tEnd);

    /// Computes the numerical fluxes for each edge of the Cartesian grid
    /**
     * The computation of fluxes strongly depends on the chosen numerical
     * method. Hence, this purely virtual function has to be implemented
     * in the respective derived classes.
     */
    virtual void computeNumericalFluxes() = 0;

    /// Computes the new values of the unknowns h, hu, and hv in all grid cells
    /**
     * Based on the numerical fluxes (computed by computeNumericalFluxes)
     * and the specified time step size dt, an Euler time step is executed.
     * As the computational fluxes will depend on the numerical method,
     * this purely virtual function has to be implemented separately for
     * each specific numerical model (and parallelisation approach).
     * @param dt Size of the time step
     */
    virtual void updateUnknowns(RealType dt) = 0;

    virtual bool hasError() = 0;

    // Access methods to grid sizes
    /// Returns #nx, i.e. the grid size in x-direction
    int getNx() const;
    /// Returns #ny, i.e. the grid size in y-direction
    int getNy() const;

    /// Returns #dx, i.e. the mesh size in x-direction
    RealType getDx() const;
    /// Returns #dy, i.e. the mesh size in y-direction
    RealType getDy() const;

    /// Returns the x-coordinate of the origin (left-bottom corner) of the Cartesian grid
    RealType getOffsetX() const;
    /// Returns the y-coordinate of the origin (left-bottom corner) of the Cartesian grid
    RealType getOffsetY() const;
  };

} // namespace Blocks
