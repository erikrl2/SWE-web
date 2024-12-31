#pragma once

#include <cstdint>
#include <string>

#include "Blocks/Block.hpp"
#include "Types/BoundaryType.hpp"
#include "Types/Float2D.hpp"
#include "Types/RealType.hpp"

namespace App {

  enum class ScenarioType;
  enum class ViewType;

  std::string scenarioTypeToString(ScenarioType type);
  std::string viewTypeToString(ViewType type);
  std::string boundaryTypeToString(BoundaryType type);

  uint32_t colorToInt(float* color4);

  RealType getScenarioValue(const Scenarios::Scenario* scenario, ViewType type, RealType x, RealType y);

  const Float2D<RealType>& getBlockValues(Blocks::Block* block, ViewType type);

  void setBlockBoundaryType(Blocks::Block* block, BoundaryType type);

} // namespace App
