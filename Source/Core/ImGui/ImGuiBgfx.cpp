/*
 * Copyright 2014-2015 Daniel Collin. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "ImGuiBgfx.hpp"

#include <bgfx/bgfx.h>
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include "imgui/fs_imgui.bin.h"
#include "imgui/fs_imgui_image.bin.h"
#include "imgui/vs_imgui.bin.h"
#include "imgui/vs_imgui_image.bin.h"

namespace Core {

  namespace ImGuiBgfx {

    // Helper function from bgfx_utils.h
    static bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices) {
      return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout) && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
    }

    static void* memAlloc(size_t _size, void* _userData);
    static void  memFree(void* _ptr, void* _userData);

    struct OcornutImguiContext {
      void render(ImDrawData* _drawData) {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int32_t dispWidth  = _drawData->DisplaySize.x * _drawData->FramebufferScale.x;
        int32_t dispHeight = _drawData->DisplaySize.y * _drawData->FramebufferScale.y;
        if (dispWidth <= 0 || dispHeight <= 0) {
          return;
        }

        bgfx::setViewName(m_viewId, "ImGui");
        bgfx::setViewMode(m_viewId, bgfx::ViewMode::Sequential);

        const bgfx::Caps* caps = bgfx::getCaps();
        {
          float ortho[16];
          float x      = _drawData->DisplayPos.x;
          float y      = _drawData->DisplayPos.y;
          float width  = _drawData->DisplaySize.x;
          float height = _drawData->DisplaySize.y;

          bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
          bgfx::setViewTransform(m_viewId, NULL, ortho);
          bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
        }

        const ImVec2 clipPos   = _drawData->DisplayPos;       // (0,0) unless using multi-viewports
        const ImVec2 clipScale = _drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int32_t ii = 0, num = _drawData->CmdListsCount; ii < num; ++ii) {
          bgfx::TransientVertexBuffer tvb;
          bgfx::TransientIndexBuffer  tib;

          const ImDrawList* drawList    = _drawData->CmdLists[ii];
          uint32_t          numVertices = (uint32_t)drawList->VtxBuffer.size();
          uint32_t          numIndices  = (uint32_t)drawList->IdxBuffer.size();

          if (!checkAvailTransientBuffers(numVertices, m_layout, numIndices)) {
            // not enough space in transient buffer just quit drawing the rest...
            break;
          }

          bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
          bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

          ImDrawVert* verts = (ImDrawVert*)tvb.data;
          bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

          ImDrawIdx* indices = (ImDrawIdx*)tib.data;
          bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

          bgfx::Encoder* encoder = bgfx::begin();

          for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd) {
            if (cmd->UserCallback) {
              cmd->UserCallback(drawList, cmd);
            } else if (0 != cmd->ElemCount) {
              uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

              bgfx::TextureHandle th      = m_texture;
              bgfx::ProgramHandle program = m_program;

              if (0 != cmd->TextureId) {
                union {
                  ImTextureID ptr;
                  struct {
                    bgfx::TextureHandle handle;
                    uint8_t             flags;
                    uint8_t             mip;
                  } s;
                } texture = {cmd->TextureId};

                state |= 0 != (0x01 & texture.s.flags) ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA) : BGFX_STATE_NONE;
                th = texture.s.handle;

                if (0 != texture.s.mip) {
                  const float lodEnabled[4] = {float(texture.s.mip), 1.0f, 0.0f, 0.0f};
                  bgfx::setUniform(u_imageLodEnabled, lodEnabled);
                  program = m_imageProgram;
                }
              } else {
                state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
              }

              // Project scissor/clipping rectangles into framebuffer space
              ImVec4 clipRect;
              clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
              clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
              clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
              clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

              if (clipRect.x < dispWidth && clipRect.y < dispHeight && clipRect.z >= 0.0f && clipRect.w >= 0.0f) {
                const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
                const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
                encoder->setScissor(xx, yy, uint16_t(bx::min(clipRect.z, 65535.0f) - xx), uint16_t(bx::min(clipRect.w, 65535.0f) - yy));

                encoder->setState(state);
                encoder->setTexture(0, s_tex, th);
                encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                encoder->submit(m_viewId, program);
              }
            }
          }

          bgfx::end(encoder);
        }
      }

      void create(void* window) {
        IMGUI_CHECKVERSION();

        static bx::DefaultAllocator allocator;
        m_allocator = &allocator;

        m_viewId     = 255;
        m_lastScroll = 0;
        m_last       = bx::getHPCounter();

        ImGui::SetAllocatorFunctions(memAlloc, memFree, NULL);

        m_imgui = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime   = 1.0f / 60.0f;
        io.IniFilename = NULL;

        setupStyle(true);

        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        switch (bgfx::getRendererType()) {
        case bgfx::RendererType::OpenGL:
          ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);
          break;
        case bgfx::RendererType::Vulkan:
          ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window, true);
          break;
        default:
          ImGui_ImplGlfw_InitForOther((GLFWwindow*)window, true);
        }

        m_program = bgfx::createProgram(bgfx::createShader(bgfx::makeRef(vs_imgui, sizeof(vs_imgui))), bgfx::createShader(bgfx::makeRef(fs_imgui, sizeof(fs_imgui))), true);

        u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
        m_imageProgram    = bgfx::createProgram(
          bgfx::createShader(bgfx::makeRef(vs_imgui_image, sizeof(vs_imgui_image))), bgfx::createShader(bgfx::makeRef(fs_imgui_image, sizeof(fs_imgui_image))), true
        );

        m_layout.begin()
          .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
          .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
          .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
          .end();

        s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

        uint8_t* data;
        int32_t  width;
        int32_t  height;

        io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);

        m_texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(data, width * height * 4));
      }

      void destroy() {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_imgui);

        bgfx::destroy(s_tex);
        bgfx::destroy(m_texture);

        bgfx::destroy(u_imageLodEnabled);
        bgfx::destroy(m_imageProgram);
        bgfx::destroy(m_program);

        m_allocator = NULL;
      }

      void setupStyle(bool _dark) {
        ImGuiStyle& style = ImGui::GetStyle();
        if (_dark) {
          ImGui::StyleColorsDark(&style);
        } else {
          ImGui::StyleColorsLight(&style);
        }

        style.FrameRounding    = 4.0f;
        style.WindowBorderSize = 0.1f;
      }

      void beginFrame() {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
      }

      void endFrame() {
        ImGui::Render();
        render(ImGui::GetDrawData());
      }

      ImGuiContext*       m_imgui;
      bx::AllocatorI*     m_allocator;
      bgfx::VertexLayout  m_layout;
      bgfx::ProgramHandle m_program;
      bgfx::ProgramHandle m_imageProgram;
      bgfx::TextureHandle m_texture;
      bgfx::UniformHandle s_tex;
      bgfx::UniformHandle u_imageLodEnabled;
      int64_t             m_last;
      int32_t             m_lastScroll;
      bgfx::ViewId        m_viewId;
    };

    static OcornutImguiContext s_ctx;

    static void* memAlloc(size_t _size, void* _userData) {
      BX_UNUSED(_userData);
      return bx::alloc(s_ctx.m_allocator, _size);
    }

    static void memFree(void* _ptr, void* _userData) {
      BX_UNUSED(_userData);
      bx::free(s_ctx.m_allocator, _ptr);
    }

    void createImGuiContext(void* window) { s_ctx.create(window); }

    void destroyImGuiContext() { s_ctx.destroy(); }

    void beginImGuiFrame() { s_ctx.beginFrame(); }

    void endImGuiFrame() { s_ctx.endFrame(); }

  } // namespace ImGuiBgfx

} // namespace Core
