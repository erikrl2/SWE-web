add_library(${SWE_PROJECT_NAME} OBJECT)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*")
list(FILTER SOURCES EXCLUDE REGEX ".*EntryPoint\\.cpp$")

# Add shaders as sources to trigger recompilation when they change
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
