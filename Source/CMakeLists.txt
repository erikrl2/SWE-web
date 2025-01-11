add_library(${SWE_PROJECT_NAME} OBJECT)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*")
list(FILTER SOURCES EXCLUDE REGEX ".*EntryPoint\\.cpp$")

# Add shaders as sources to trigger recompilation when they change
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/Assets/Shaders CACHE PATH "Path to shader files")
file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS "${SHADER_DIR}/*.sc")

target_sources(${SWE_PROJECT_NAME} PRIVATE ${SOURCES} ${SHADER_SOURCES})

target_link_libraries(${SWE_PROJECT_NAME} PUBLIC SWE-Interface)

target_include_directories(${SWE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SHADER_DIR}
)

add_dependencies(${SWE_PROJECT_NAME} compile_shaders)

add_executable(${SWE_PROJECT_NAME}-App Core/EntryPoint.cpp)
target_link_libraries(${SWE_PROJECT_NAME}-App PRIVATE ${SWE_PROJECT_NAME})

if(EMSCRIPTEN)
  target_link_options(${SWE_PROJECT_NAME}-App PRIVATE
    -sMAX_WEBGL_VERSION=2
    -sGL_ENABLE_GET_PROC_ADDRESS
    -sALLOW_MEMORY_GROWTH=1
    -sMAX_WEBGL_VERSION=2
    -sUSE_GLFW=3
    --shell-file=${CMAKE_SOURCE_DIR}/Source/Shell.html
    --preload-file ../../Assets/data@/data
  )
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()
