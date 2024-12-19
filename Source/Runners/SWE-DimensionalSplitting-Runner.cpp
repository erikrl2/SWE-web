/**
 * @file SWE-DimensionalSplitting-Runner.cpp
 * @brief Main program for the Shallow Water Equations solver using dimensional splitting.
 */

#include <iostream>

#include "Blocks/Block.hpp"
#include "Blocks/DimensionalSplitting.hpp"
#include "Scenarios/ArtificialTsunamiScenario.hpp"
#include "Scenarios/TestScenario.hpp"

int mainNotUsed() {
  std::string  baseName            =  "SWE";
  int          numberOfGridCellsX  =  64;
  int          numberOfGridCellsY  =  64;
  int          numberOfCheckPoints =  100;
  double       endSimulationTime   =  100.0;
  BoundaryType boundaryCondition   = (BoundaryType) 0;
  int          flushCount          =  10;

  // Load scenario
  Scenarios::Scenario* scenario = nullptr;

  enum class ScenarioType { LoadCheckpoint, Tsunami, ArtificialTsunami, Test } scenarioType;
  scenarioType = ScenarioType::ArtificialTsunami;

  switch (scenarioType) {
  case ScenarioType::LoadCheckpoint: {
    // auto s = new Scenarios::LoadCheckpointScenario(baseName + ".nc");

    // if (s->success()) {

    //   coarseness          = s->getCoarseness();
    //   numberOfGridCellsX  = s->getNumberOfCellsX();
    //   numberOfGridCellsY  = s->getNumberOfCellsY();
    //   numberOfCheckPoints = s->getTotalNumberOfCheckpoints();

    //   simulationTimeOffset = s->getCurrentSimulationTime();
    //   checkPointOffset     = s->getCurrentCheckpointNumber();

    //   scenario = s;
    // } else {
    //   std::cerr << "Error loading checkpoint file" << std::endl;
    //   delete s;
    //   return EXIT_FAILURE;
    // }
    // break;
  }
  case ScenarioType::Tsunami: {
    // std::string bathymetryFileName   = args.getArgument<std::string>("bathymetry-file");
    // std::string displacementFileName = args.getArgument<std::string>("displacement-file");

    // if (Writers::Writer::fileExists(bathymetryFileName) && Writers::Writer::fileExists(displacementFileName)) {
    //   auto s = new Scenarios::TsunamiScenario(bathymetryFileName, displacementFileName, endSimulationTime, boundaryCondition, numberOfGridCellsX, numberOfGridCellsY);

    //   if (s->success()) {
    //     scenario = s;
    //   } else {
    //     std::cerr << "Error loading Tsunami Scenario" << std::endl;
    //     delete s;
    //     return EXIT_FAILURE;
    //   }

    // } else {
    //   std::cerr << "Bathymetry or displacement file does not exist." << std::endl;
    //   return EXIT_FAILURE;
    // }
    // break;
  }
  case ScenarioType::ArtificialTsunami: {
    scenario = new Scenarios::ArtificialTsunamiScenario(endSimulationTime, boundaryCondition);
    break;
  }
  case ScenarioType::Test: {
    auto s             = new Scenarios::TestScenario(8);
    numberOfGridCellsX = s->getCellCount();
    numberOfGridCellsY = s->getCellCount();
    scenario           = s;
    break;
  }
  }

  const RealType boundaryPos[4] = {
    // Left=0, Right=1, Bottom=2, Top=3
    scenario->getBoundaryPos(BoundaryEdge::Left),
    scenario->getBoundaryPos(BoundaryEdge::Right),
    scenario->getBoundaryPos(BoundaryEdge::Bottom),
    scenario->getBoundaryPos(BoundaryEdge::Top)
  };

  // Compute the size of a single cell
  RealType cellSizeX = (boundaryPos[BoundaryEdge::Right] - boundaryPos[BoundaryEdge::Left]) / numberOfGridCellsX;
  RealType cellSizeY = (boundaryPos[BoundaryEdge::Top] - boundaryPos[BoundaryEdge::Bottom]) / numberOfGridCellsY;

  auto waveBlock = new Blocks::DimensionalSplitting(numberOfGridCellsX, numberOfGridCellsY, cellSizeX, cellSizeY);

  // Get the origin from the scenario
  RealType originX = boundaryPos[BoundaryEdge::Left];
  RealType originY = boundaryPos[BoundaryEdge::Bottom];

  // Initialise the wave propagation block
  waveBlock->initialiseScenario(originX, originY, *scenario);

  // Get the final simulation time from the scenario
  endSimulationTime = scenario->getEndSimulationTime();

  delete scenario; // free memory as soon as possible

  // Checkpoints when output files are written
  double* checkPoints = new double[numberOfCheckPoints + 1]{0};

  // Compute the checkpoints in time
  for (int cp = 1; cp <= numberOfCheckPoints; cp++) {
    checkPoints[cp] = cp * (endSimulationTime / numberOfCheckPoints);
  }

  // Flush checkpoint file x times
  if (flushCount > 0) {
    flushCount = (flushCount <= numberOfCheckPoints) ? numberOfCheckPoints / flushCount : 1;
  }

  double simulationTime = 0.0;

  // Loop over checkpoints
  for (int cp = 1; cp <= numberOfCheckPoints; cp++) {
    // Do time steps until next checkpoint is reached
    while (simulationTime < checkPoints[cp]) {
      // Set values in ghost cells
      waveBlock->setGhostLayer();

      // Compute numerical flux on each edge
      waveBlock->computeNumericalFluxes();

      RealType maxTimeStepWidth = waveBlock->getMaxTimeStep();

      // Update the cell values
      waveBlock->updateUnknowns(maxTimeStepWidth);

      // Update simulation time with time step width
      simulationTime += maxTimeStepWidth;
    }

    // TODO: Render frame
  }

  delete waveBlock;
  delete[] checkPoints;

  return EXIT_SUCCESS;
}
