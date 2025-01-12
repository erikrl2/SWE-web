/**
 * @file Fwave.cpp
 * @brief Implementation of the F-Wave Solver for shallow water equations.
 *
 * This file implements the net updates computation for the F-Wave Solver,
 * which simulates shallow water equations by calculating left-going and
 * right-going wave effects.
 */
#include "Solvers/Fwave.hpp"

#include <cassert>
#include <cmath>

#define EXIT_IF_NOT(condition) \
  if (!(condition)) { \
    Error = true; \
    return; \
  }

namespace Solvers {

  void Fwave::computeNetUpdates(
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
  ) {

    const RealType g = 9.81; // Gravitation constant

    // Handle cases with dry cells
    bool isDryLeft  = bLeft > RealType(0.0);
    bool isDryRight = bRight > RealType(0.0);

    if (isDryLeft && isDryRight) { // Dry-Dry
      o_hUpdateLeft = o_hUpdateRight = o_huUpdateLeft = o_huUpdateRight = o_maxWaveSpeed = RealType(0.0);
      return;
    } else if (isDryLeft) { // Dry-Wet
      hLeft  = hRight;
      bLeft  = bRight;
      huLeft = -huRight;
    } else if (isDryRight) { // Wet-Dry
      hRight  = hLeft;
      bRight  = bLeft;
      huRight = -huLeft;
    }

    // assert that heights are positive
    EXIT_IF_NOT(hLeft > RealType(0.0));
    EXIT_IF_NOT(hRight > RealType(0.0));

    /**
     * Step 1 / 2: The initial discontinuity at x = 0 evolves into two waves, which can either be shock waves or rarefaction waves.
     * We approximate these waves using the Roe linearization:
     */

    // Compute square roots for Roe averages
    RealType sqrt_hL = std::sqrt(hLeft);
    RealType sqrt_hR = std::sqrt(hRight);

    // Avoid division by zero
    RealType denom = sqrt_hL + sqrt_hR;
    assert(denom > 1e-8); // Ensure denominator is not too small

    // Compute velocities
    RealType uL = (hLeft > RealType(0.0)) ? (huLeft / hLeft) : RealType(0.0);
    RealType uR = (hRight > RealType(0.0)) ? (huRight / hRight) : RealType(0.0);

    /**
     * Step 4: Compute Roe Averages for Height and Velocity
     * Calculate the Roe-averaged height (hRoe) and velocity (uRoe) using the values from the left and right states.
     */
    RealType uRoe = (sqrt_hL * uL + sqrt_hR * uR) / denom; // Roe-averaged velocity
    RealType hRoe = 0.5 * (hLeft + hRight);                // Roe-averaged height

    /**
     * Step 3: Approximate Wave Speeds using Roe Eigenvalues
     * Calculate the Roe-averaged quantities and use them to approximate the wave speeds, denoted as λ₁ᴿᵒᵉ and λ₂ᴿᵒᵉ.
     */
    RealType cRoe    = std::sqrt(g * hRoe); // g (gravitation * hRoe is the Roe-averaged height
    RealType lambda1 = uRoe - cRoe;         // left-going  wave speeds
    RealType lambda2 = uRoe + cRoe;         // right-going wave speeds

    lambda1 = std::fmin(lambda1, uL - std::sqrt(g * hLeft));
    lambda2 = std::fmax(lambda2, uR + std::sqrt(g * hRight));

    /**
     * Step 5: Construct the eigenvectors r₁ᴿᵒᵉ and r₂ᴿᵒᵉ based on the calculated Roe eigenvalues.
     * The eigenvectors describe the direction in which each characteristic wave propagates. They are essential for decomposing the flux difference and determining the influence of
     * each wave on the solution. Each eigenvector has two components: the first component is always 1, and the second component is the corresponding eigenvalue (𝜆1 or 𝜆2).
     */
    RealType r1[2] = {1.0, lambda1};
    RealType r2[2] = {1.0, lambda2};

    /**
     * Step 6: Decompose the Flux Difference (Δf)
     * Decompose the difference in fluxes between the right and left states into components along the Roe eigenvectors.
     * This decomposition identifies the contributions of each wave (denoted as Z₁ and Z₂).
     * The goal is to find the difference in these fluxes between the left and right states and adjust this difference by the source term (due to bathymetry changes).
     */
    // Flux Calculation: f = [hu, hu² + 0.5 * g * h²]
    RealType fL[2] = {huLeft, uL * huLeft + RealType(0.5) * g * hLeft * hLeft};
    RealType fR[2] = {huRight, uR * huRight + RealType(0.5) * g * hRight * hRight};

    // Compute flux difference
    RealType delta_f[2] = {
      fR[0] - fL[0], // Mass flux difference
      fR[1] - fL[1]  // Momentum flux difference
    };

    // Compute difference in bathymetry source term
    RealType delta_b = bRight - bLeft;
    RealType s[2]    = {RealType(0.0), -g * RealType(0.5) * (hLeft + hRight) * delta_b};

    // Adjust flux difference by source term
    delta_f[0] -= s[0];
    delta_f[1] -= s[1];

    /**
     * Step 8: Compute Eigenvalue Coefficients (α₁ and α₂)
     * Solve for the coefficients α₁ and α₂, which weight the eigenvectors in the decomposition of the flux difference.
     * These coefficients determine the amplitude of each wave component in the solution.
     */
    // Calculate the Coefficients 𝛼1 and 𝛼2: These coefficients are derived using the eigenvalues and the flux difference:
    RealType denominator = lambda2 - lambda1;
    assert(std::abs(denominator) > RealType(1e-8)); // Avoid division by zero
    RealType alpha1 = (lambda2 * delta_f[0] - delta_f[1]) / denominator;
    RealType alpha2 = (-lambda1 * delta_f[0] + delta_f[1]) / denominator;

    // The wave contributions are calculated by scaling the eigenvectors r1 and r2 by their respective coefficients:
    RealType Z1[2] = {alpha1 * r1[0], alpha1 * r1[1]};
    RealType Z2[2] = {alpha2 * r2[0], alpha2 * r2[1]};

    /**
     * Step 7: Calculate Net Updates for the Left and Right Cells
     * Determine the net effect of the waves on the left and right cells by summing the contributions of waves moving into each cell.
     * Left-going waves affect the left cell, and right-going waves affect the right cell.
     */
    // Initialize Net Updates: Start with zero contributions to both the left and right cells:
    o_hUpdateLeft   = RealType(0.0);
    o_huUpdateLeft  = RealType(0.0);
    o_hUpdateRight  = RealType(0.0);
    o_huUpdateRight = RealType(0.0);

    // Assign Contributions Based on Wave Direction: If λ1 <0, the first wave (𝑍1) is moving towards the left, so it affects the left cell. Similarly, if 𝜆1>0, the first wave
    // affects the right cell:
    if (lambda1 < RealType(0.0)) {
      o_hUpdateLeft += Z1[0];
      o_huUpdateLeft += Z1[1];
    } else if (lambda1 > RealType(0.0)) {
      o_hUpdateRight += Z1[0];
      o_huUpdateRight += Z1[1];
    }

    // Similarly, the second wave (𝑍2) affects the left or right cell depending on the sign of 𝜆2:
    if (lambda2 < RealType(0.0)) {
      o_hUpdateLeft += Z2[0];
      o_huUpdateLeft += Z2[1];
    } else if (lambda2 > RealType(0.0)) {
      o_hUpdateRight += Z2[0];
      o_huUpdateRight += Z2[1];
    }

    // Ensure that one of the net-updates is zero in supersonic cases
    if (lambda1 == RealType(0.0)) {
      assert(o_hUpdateLeft == RealType(0.0) && o_huUpdateLeft == RealType(0.0));
    }
    if (lambda2 == RealType(0.0)) {
      assert(o_hUpdateRight == RealType(0.0) && o_huUpdateRight == RealType(0.0));
    }

    // Compute the maximum wave speed
    o_maxWaveSpeed = std::fmax(std::abs(lambda1), std::abs(lambda2));

    // Set updates to zero for dry cells
    if (isDryLeft) {
      o_hUpdateLeft = o_huUpdateLeft = RealType(0.0);
    }
    if (isDryRight) {
      o_hUpdateRight = o_huUpdateRight = RealType(0.0);
    }
  }

} // namespace Solvers
