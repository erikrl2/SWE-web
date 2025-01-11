#include "Utils.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>

namespace App {

  std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
#ifndef __EMSCRIPTEN__
    case ScenarioType::Tsunami:
      return "Tsunami";
#endif
    case ScenarioType::ArtificialTsunami:
      return "Artificial Tsunami";
    case ScenarioType::RealisticTsunami:
      return "Realistic Tsunami";
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

  RealType getBlockValue(const Blocks::Block* block, ViewType type, int i, int j) {
    assert(i >= 1 && i <= block->getNx());
    assert(j >= 1 && j <= block->getNy());

    switch (type) {
    case ViewType::H:
      return block->getWaterHeight()[j][i];
    case ViewType::Hu:
      return block->getDischargeHu()[j][i];
    case ViewType::Hv:
      return block->getDischargeHv()[j][i];
    case ViewType::B:
      return block->getBathymetry()[j][i];
    case ViewType::HPlusB:
      return block->getWaterHeight()[j][i] + block->getBathymetry()[j][i];
    default:
      assert(false);
    }
    return 0; // dummy
  }

  RealType getBlockValue(const Blocks::Block* block, ViewType type, RealType x, RealType y) {
    int i = std::round((x - block->getOffsetX()) / block->getDx() + RealType(0.5));
    int j = std::round((y - block->getOffsetY()) / block->getDy() + RealType(0.5));
    return getBlockValue(block, type, i, j);
  }

  void setBlockBoundaryType(Blocks::Block* block, BoundaryType type) {
    if (block) {
      block->setBoundaryType(BoundaryEdge::Left, type);
      block->setBoundaryType(BoundaryEdge::Right, type);
      block->setBoundaryType(BoundaryEdge::Bottom, type);
      block->setBoundaryType(BoundaryEdge::Top, type);
    }
  }

  Vec3f toVec3f(bx::Vec3 v) { return {v.x, v.y, v.z}; }

  bx::Vec3 toBxVec3(Vec3f v) { return {v.x, v.y, v.z}; }

  std::string removeDriveLetter(const std::string& path) {
    std::filesystem::path fsPath(path);
    if (fsPath.has_root_name() && fsPath.root_name().string().size() > 1) {
      std::string modifiedPath = fsPath.string();
      size_t      pos          = modifiedPath.find(':');
      if (pos != std::string::npos) {
        modifiedPath.erase(0, pos + 1);
      }
      return modifiedPath;
    }
    return path;
  }

} // namespace App
