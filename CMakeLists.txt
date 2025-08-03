cmake_minimum_required(VERSION 3.29.6)
project(rznode VERSION 0.1.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS ON)

set(OUT_BINARY_DIR ${CMAKE_SOURCE_DIR}/Binaries/${CMAKE_BUILD_TYPE})
set(OUTPUT_DIR
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OUT_BINARY_DIR}"
)

include(cmake/AddLibrary.cmake)
include(cmake/AddNodes.cmake)

add_subdirectory(ext/googletest)
add_subdirectory(ext/entt)
add_subdirectory(ext/spdlog)

add_subdirectory(core)
add_subdirectory(system)
add_subdirectory(ui_imgui)
