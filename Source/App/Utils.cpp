#include "Utils.hpp"

#include <cassert>

#include "SweApp.hpp"

namespace App {

  std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami:
      return "Tsunami";
#endif
    case ScenarioType::ArtificialTsunami:
      return "Artificial Tsunami";
    case ScenarioType::Test:
      return "Test";
    case ScenarioType::None:
      return "None";
    default:
      assert(false);
    }
    return {};
  }

  std::string viewTypeToString(ViewType type) {
    switch (type) {
    case ViewType::H:
      return "Water Height h";
    case ViewType::Hu:
      return "Water Momentum hu";
    case ViewType::Hv:
      return "Water Momentum hv";
    case ViewType::B:
      return "Bathymetry b";
    case ViewType::HPlusB:
      return "Water Height + Bathymetry)";
    default:
      assert(false);
    }
    return {};
  }

  std::string boundaryTypeToString(BoundaryType type) {
    switch (type) {
    case BoundaryType::Wall:
      return "Wall";
    case BoundaryType::Outflow:
      return "Outflow";
    default:
      assert(false);
    }
    return {};
  }

  uint32_t colorToInt(float* color4) {
    uint32_t color = 0;
    for (int i = 0; i < 4; i++) {
      color |= (uint32_t)(color4[i] * 255) << ((3 - i) * 8);
    }
    return color;
  }

  RealType getScenarioValue(const Scenarios::Scenario* scenario, ViewType type, RealType x, RealType y) {
    switch (type) {
    case ViewType::H:
      return scenario->getWaterHeight(x, y);
    case ViewType::Hu:
      return scenario->getMomentumU(x, y);
    case ViewType::Hv:
      return scenario->getMomentumV(x, y);
    case ViewType::B:
      return scenario->getBathymetry(x, y);
    case ViewType::HPlusB:
      return scenario->getWaterHeight(x, y) + scenario->getBathymetry(x, y);
    default:
      assert(false);
    }
    return 0; // dummy
  }

  const Float2D<RealType>& getBlockValues(Blocks::Block* block, ViewType type) {
    switch (type) {
    case ViewType::H:
      return block->getWaterHeight();
    case ViewType::Hu:
      return block->getDischargeHu();
    case ViewType::Hv:
      return block->getDischargeHv();
    case ViewType::B:
      return block->getBathymetry();
    default:
      assert(false);
    }
    return block->getWaterHeight(); // dummy
  }


  void setBlockBoundaryType(Blocks::Block* block, BoundaryType type) {
    if (block) {
      block->setBoundaryType(BoundaryEdge::Left, type);
      block->setBoundaryType(BoundaryEdge::Right, type);
      block->setBoundaryType(BoundaryEdge::Bottom, type);
      block->setBoundaryType(BoundaryEdge::Top, type);
    }
  }

} // namespace App
