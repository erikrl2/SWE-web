add_library(${SWE_PROJECT_NAME} OBJECT)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*")
list(FILTER SOURCES EXCLUDE REGEX ".*EntryPoint\\.cpp$")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

set(ASSETS_DIR ${CMAKE_SOURCE_DIR}/Assets CACHE PATH "Path to Assets directoy")

file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS "${ASSETS}/Shaders/*.sc")

target_sources(${SWE_PROJECT_NAME} PRIVATE ${SOURCES} ${SHADER_SOURCES})

target_link_libraries(${SWE_PROJECT_NAME} PUBLIC SWE-Interface)

target_include_directories(${SWE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ASSETS_DIR}
)

add_dependencies(${SWE_PROJECT_NAME} compile_shaders)

add_executable(${SWE_PROJECT_NAME}-App Core/EntryPoint.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_link_options(${SWE_PROJECT_NAME}-App PRIVATE
    -sMAX_WEBGL_VERSION=2
    -sGL_ENABLE_GET_PROC_ADDRESS
    -sALLOW_MEMORY_GROWTH=1
    -sMAX_WEBGL_VERSION=2
    -sUSE_GLFW=3
    -fwasm-exceptions
    --shell-file=${CMAKE_SOURCE_DIR}/Source/Shell.html
    --preload-file ${CMAKE_BINARY_DIR}/Assets/Data@/Assets/Data
  )
  target_compile_options(${SWE_PROJECT_NAME} PRIVATE -fwasm-exceptions)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(${SWE_PROJECT_NAME}-App
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${SWE_PROJECT_NAME}-App>")
endif()

target_link_libraries(${SWE_PROJECT_NAME}-App PRIVATE ${SWE_PROJECT_NAME})

add_custom_command(
  TARGET ${SWE_PROJECT_NAME}-App PRE_LINK
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/Assets/Data
  $<TARGET_FILE_DIR:${SWE_PROJECT_NAME}-App>/Assets/Data
)
