cmake_minimum_required(VERSION 3.29.6)
project(rznode VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(RZNODES_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "RZNodes directory" FORCE)
message(STATUS "RZNODES_DIR: ${RZNODES_DIR}")

set(BUILD_SHARED_LIBS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(NOT OUT_BINARY_DIR)
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
endif()

if(NOT Python_FOUND)
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    if(NOT Python_EXECUTABLE)
        set(Python_EXECUTABLE ${Python_INTERPRETER})
    endif()
endif()

if(NOT Python3_FOUND)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    if(NOT Python3_EXECUTABLE)
        set(Python3_EXECUTABLE ${Python3_INTERPRETER})
    endif()
endif()

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
if(nanobind_ROOT)
    find_package(nanobind CONFIG REQUIRED)
endif()

include(cmake/AddLibrary.cmake)
include(cmake/AddNodes.cmake)
enable_testing()

if (NOT TARGET gtest)
add_subdirectory(ext/googletest)

set_target_properties(gtest PROPERTIES FOLDER "ThirdParty")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty")

set_target_properties(gtest PROPERTIES ${OUTPUT_DIR})
set_target_properties(gtest_main PROPERTIES ${OUTPUT_DIR})

endif()

add_subdirectory(ext/entt)
add_subdirectory(ext/spdlog)

set_target_properties(spdlog PROPERTIES ${OUTPUT_DIR})

add_subdirectory(core)
add_subdirectory(system)

option(NODE_IMGUI_ENABLED "Enable ImGui UI Nodes" OFF)
if(NODE_IMGUI_ENABLED)
    add_subdirectory(ui_imgui)
endif()

option(NODE_WEB_SERVER_ENABLED "Enable Web Server with BaklavaJS integration" ON)
option(GEOM_EXTENSION_ENABLED "Enable GCore geometry extension for web server" ON)
if(NODE_WEB_SERVER_ENABLED)
    # oatpp version
    set(BUILD_SHARED_LIBS OFF)  # 将 OATPP 构建为静态库
    set(OATPP_INSTALL OFF CACHE BOOL "Disable installation of oatpp")
    set(OATPP_BUILD_TESTS OFF CACHE BOOL "Disable building tests for oatpp")
    set(OATPP_LINK_TEST_LIBRARY OFF CACHE BOOL "Disable linking test library for oatpp")
    add_subdirectory(ext/oatpp)
    set_target_properties(oatpp PROPERTIES ${OUTPUT_DIR})

    set(OATPP_MODULES_LOCATION "CUSTOM" CACHE STRING "Set custom location for oatpp modules")
    set(OATPP_DIR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/ext/oatpp/" CACHE PATH "oatpp source directory")
    set(OATPP_DIR_LIB "${CMAKE_CURRENT_BINARY_DIR}/ext/oatpp/" CACHE PATH "oatpp build directory")
    add_subdirectory(ext/oatpp-websocket)
    set_target_properties(oatpp-websocket PROPERTIES ${OUTPUT_DIR})
    set(BUILD_SHARED_LIBS ON)  # 恢复 BUILD_SHARED_LIBS 选项

    add_subdirectory(web_server_oatpp)
    if(GEOM_EXTENSION_ENABLED)
        add_subdirectory(ext/GCore)
        target_link_libraries(web_server_oatpp PUBLIC geometry)
        target_include_directories(web_server_oatpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/web_server_oatpp/geom_extension/include)
        target_compile_definitions(web_server_oatpp PRIVATE GEOM_EXTENSION=1)
    endif()

endif()

add_subdirectory(applications)