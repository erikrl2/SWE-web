#include "Utils.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <imgui.h>

namespace App {

  std::string scenarioTypeToString(ScenarioType type) {
    switch (type) {
#ifdef ENABLE_NETCDF
    case ScenarioType::NetCDF:
      return "NetCDF";
#endif
    case ScenarioType::Tohoku:
      return "Tohoku";
    case ScenarioType::TohokuZoomed:
      return "Tohoku (Zoomed)";
    case ScenarioType::Chile:
      return "Chile";
    case ScenarioType::ArtificialTsunami:
      return "Artificial";
#ifndef NDEBUG
    case ScenarioType::Test:
      return "Test";
#endif
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
      return "Water Height + Bathymetry";
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

  bool fileExists(const std::string& path) {
    std::filesystem::path fsPath(path);
    return std::filesystem::exists(fsPath);
  }

  Vec2f getInitialZValueScale(ScenarioType type) {
    switch (type) {
#ifdef ENABLE_NETCDF
    case ScenarioType::NetCDF:
      return {20.0f, 20.0f};
#endif
    case ScenarioType::Tohoku:
      return {40.0f, 40.0f};
    case ScenarioType::TohokuZoomed:
      return {20.0f, 20.0f};
    case ScenarioType::Chile:
      return {100.0f, 100.0f};
    case ScenarioType::ArtificialTsunami:
      return {100000.0f, 0.0f};
#ifndef NDEBUG
    case ScenarioType::Test:
      return {1.0f, 0.0f};
#endif
    case ScenarioType::None:
      return {0.0f, 0.0f};
    default:
      assert(false);
    }
    return {};
  }

  bool drawCoordinatePicker2D(const char* label, Vec2f& coords, const Vec2f& size) {
    ImGui::InvisibleButton(label, ImVec2(size.x, size.y));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2      p0       = ImGui::GetItemRectMin();
    ImVec2      p1       = ImGui::GetItemRectMax();

    drawList->AddRect(p0, p1, IM_COL32(255, 255, 255, 255));

    Vec2f normCoords = {0.5f + coords.x, 0.5f - coords.y};

    if (ImGui::IsItemActive()) {
      ImVec2 mousePos = ImGui::GetMousePos();
      normCoords.x    = (mousePos.x - p0.x) / (p1.x - p0.x);
      normCoords.y    = (mousePos.y - p0.y) / (p1.y - p0.y);
      normCoords.x    = bx::clamp(normCoords.x, 0.0f, 1.0f);
      normCoords.y    = bx::clamp(normCoords.y, 0.0f, 1.0f);

      coords.x = normCoords.x - 0.5f;
      coords.y = 0.5f - normCoords.y;
    }

    ImVec2 cursorPos = ImVec2(p0.x + normCoords.x * (p1.x - p0.x), p0.y + normCoords.y * (p1.y - p0.y));
    drawList->AddCircleFilled(cursorPos, 2.5f, IM_COL32(255, 0, 0, 255));

    return (ImGui::IsItemHovered() || ImGui::IsItemActive()) && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
  }

} // namespace App
