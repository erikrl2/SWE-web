#pragma once

#include <cstdint>
#include <string>

enum class BoundaryType;

namespace App {

  enum class ScenarioType;
  enum class ViewType;

  std::string scenarioTypeToString(ScenarioType type);
  std::string viewTypeToString(ViewType type);
  std::string boundaryTypeToString(BoundaryType type);

  uint32_t colorToInt(float* color4);

} // namespace App
