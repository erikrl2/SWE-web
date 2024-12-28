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

} // namespace App
