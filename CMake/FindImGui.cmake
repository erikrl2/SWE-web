include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.6
)
cmake_policy(SET CMP0169 OLD)
FetchContent_Populate(imgui)

file(WRITE "${imgui_SOURCE_DIR}/CMakeLists.txt" [[
cmake_minimum_required(VERSION 3.24)

add_library(imgui STATIC
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/backends
)
]])

add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR})
