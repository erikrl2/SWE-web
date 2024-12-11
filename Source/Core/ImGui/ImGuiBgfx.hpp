#pragma once

#include <bgfx/bgfx.h>
#include <imgui.h>

namespace Core {

  namespace ImGuiBgfx {

    void createImGuiContext(void* window);
    void destroyImGuiContext();

    void beginImGuiFrame();
    void endImGuiFrame();

  } // namespace ImGuiBgfx

} // namespace Core
