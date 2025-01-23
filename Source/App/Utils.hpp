#pragma once

#include <bx/math.h>
#include <cstdint>
#include <string>

#include "Blocks/Block.hpp"
#include "Types/Common.hpp"

namespace App {

  std::string scenarioTypeToString(ScenarioType type);
  std::string viewTypeToString(ViewType type);
  std::string boundaryTypeToString(BoundaryType type);

  uint32_t colorToInt(float* color4);

  RealType getScenarioValue(const Scenarios::Scenario* scenario, ViewType type, RealType x, RealType y);

  RealType getBlockValue(const Blocks::Block* block, ViewType type, int i, int j);
  RealType getBlockValue(const Blocks::Block* block, ViewType type, RealType x, RealType y);

  void setBlockBoundaryType(Blocks::Block* block, BoundaryType type);

  Vec3f    toVec3f(bx::Vec3 v);
  bx::Vec3 toBxVec3(Vec3f v);

  std::string removeDriveLetter(const std::string& path);

  bool fileExists(const std::string& path);

  Vec2f getInitialZValueScale(ScenarioType type);

} // namespace App
