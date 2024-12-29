#pragma once

#include <cstdint>
#include <string>

#include "Types/Float2D.hpp"
#include "Types/RealType.hpp"

enum class BoundaryType;

namespace Blocks {
  class Block;
} // namespace Blocks

namespace App {

  enum class ScenarioType;
  enum class ViewType;

  std::string scenarioTypeToString(ScenarioType type);
  std::string viewTypeToString(ViewType type);
  std::string boundaryTypeToString(BoundaryType type);

  uint32_t colorToInt(float* color4);

  const Float2D<RealType>& getBlockValues(Blocks::Block* block, ViewType type);

} // namespace App
