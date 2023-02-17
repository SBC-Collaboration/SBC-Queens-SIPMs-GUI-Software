cmake_minimum_required(VERSION 3.14...3.22)

project(SBCQueensGUI LANGUAGES CXX)

file(GLOB GUI_CONFIG_FILE ../gui_setup.toml)
file(COPY ${GUI_CONFIG_FILE} DESTINATION ${PROJECT_BINARY_DIR})

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
CPMAddPackage(NAME SBCQueensGUIHelpers SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----
if(USE_VULKAN)
  add_definitions(-DUSE_VULKAN)
endif()

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set(CMAKE_CXX_STANDARD 20)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME
  "SBCQueensGUI")
target_link_libraries(${PROJECT_NAME} PUBLIC SBCQueensGUIHelpers)
