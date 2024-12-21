cmake_minimum_required(VERSION 3.24)

set(SWE_PROJECT_NAME "SWE")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_BRANCH GIT_COMMIT_HASH)
get_git_commit_message(GIT_COMMIT_MESSAGE)
get_git_commit_timestamp(GIT_COMMIT_TIMESTAMP)
message(STATUS "Revision: ${GIT_BRANCH} -m \"${GIT_COMMIT_MESSAGE}\" (${GIT_COMMIT_HASH}) ${GIT_COMMIT_TIMESTAMP}")

project(${SWE_PROJECT_NAME})

set(CMAKE_C_STANDARD 17 CACHE STRING "")
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  message(STATUS "Configuring for Web using Emscripten")
  set(EMSCRIPTEN TRUE)
  set(ENABLE_OPENMP OFF)
else()
  message(STATUS "Configuring for Desktop")
  set(EMSCRIPTEN FALSE)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type: Debug, Release, RelWithDebInfo, MinSizeRel")
  message(STATUS "Build type not specified: defaulting to Debug")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")

include(GNUInstallDirs)
set(DEFAULT_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_library(SWE-Interface INTERFACE)
target_compile_features(SWE-Interface INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

target_compile_options(SWE-Interface INTERFACE
  -W
  -Wall
  -Wextra
  -Wpedantic
)

option(ENABLE_SINGLE_PRECISION "Enable single floating-point precision" OFF)
if(ENABLE_SINGLE_PRECISION)
  target_compile_definitions(SWE-Interface INTERFACE ENABLE_SINGLE_PRECISION)
endif()

option(ENABLE_OPENMP "Enable shared-memory parallelization using OpenMP" ON)
if(ENABLE_OPENMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(SWE-Interface INTERFACE OpenMP::OpenMP_CXX)
  target_compile_definitions(SWE-Interface INTERFACE ENABLE_OPENMP)
endif()

if(NOT EMSCRIPTEN)
  find_package(glfw3 REQUIRED)
  find_package(NetCDF REQUIRED)
  target_include_directories(SWE-Interface INTERFACE ${NETCDF_INCLUDES})
  target_link_libraries(SWE-Interface INTERFACE glfw ${NETCDF_LIBRARIES})
endif()

find_package(bgfx REQUIRED)
find_package(ImGui REQUIRED)
target_link_libraries(SWE-Interface INTERFACE bgfx bx imgui)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders CACHE PATH "Path to shader files")
add_custom_target(
    compile_shaders ALL
    COMMAND ${CMAKE_MAKE_PROGRAM} -f shader.mk SHADER_DIR=${SHADER_DIR}/ BGFX_DIR=${BGFX_DIR}/ | tee
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Scripts
    COMMENT "Compiling shaders to header files using shader.mk"
)

add_subdirectory(Source)
