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

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
  message(STATUS "Build type not specified: defaulting to ${CMAKE_BUILD_TYPE}")
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

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(SWE-Interface INTERFACE /W4)
else()
  target_compile_options(SWE-Interface INTERFACE -W -Wall -Wextra -Wpedantic)
endif()

option(ENABLE_SINGLE_PRECISION "Enable single floating-point precision" OFF)
if(ENABLE_SINGLE_PRECISION)
  target_compile_definitions(SWE-Interface INTERFACE ENABLE_SINGLE_PRECISION)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(STATUS "Emscripten detected")
  set(PLATFORM asm.js)
  set(SC_PROFILE 300_es) # OpenGLES
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(STATUS "Windows detected")
  set(PLATFORM windows)
  set(SC_PROFILE "s_5_0 -O3") # Direct3D
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  message(STATUS "macOS detected")
  set(PLATFORM osx)
  set(SC_PROFILE metal) # Metal
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "Linux detected")
  set(PLATFORM linux)
  set(SC_PROFILE 130) # OpenGL
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

set(ALLOW_NETCDF OFF)
if(NOT EMSCRIPTEN OR VCPKG_TOOLCHAIN)
  set(ALLOW_NETCDF ON)
endif()

include(CMakeDependentOption)
cmake_dependent_option(ENABLE_NETCDF "Enable loading NetCDF input files." ON ALLOW_NETCDF OFF)
if(ENABLE_NETCDF)
  if(VCPKG_TOOLCHAIN)
    find_package(netCDF CONFIG REQUIRED)
    find_package(netCDFCxx CONFIG REQUIRED)
    target_link_libraries(SWE-Interface INTERFACE netCDF::netcdf-cxx4)
  else() # search for netCDF installed on the system
    find_package(netCDF REQUIRED)
    target_include_directories(SWE-Interface INTERFACE ${NETCDF_INCLUDES})
    target_link_libraries(SWE-Interface INTERFACE ${NETCDF_LIBRARIES})
  endif()
  target_compile_definitions(SWE-Interface INTERFACE ENABLE_NETCDF)
endif()

if(NOT EMSCRIPTEN)
  find_package(glfw3 REQUIRED)
  target_link_libraries(SWE-Interface INTERFACE glfw)
endif()

find_package(bgfx REQUIRED)
find_package(imgui REQUIRED)
target_link_libraries(SWE-Interface INTERFACE bgfx bx imgui)

# NOTE: Clean header files with `make -f Scripts/shader.mk clean` before building on a different platform
add_custom_target(
  compile_shaders ALL
  COMMAND make -f shader.mk PLATFORM=${PLATFORM} PROFILE=${SC_PROFILE} BGFX_DIR=${BGFX_DIR}/ | tee
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Scripts
  COMMENT "Compiling shaders to header files"
)

add_subdirectory(Source)
