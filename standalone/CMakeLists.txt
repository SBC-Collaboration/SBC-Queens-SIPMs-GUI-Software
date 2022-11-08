cmake_minimum_required(VERSION 3.14...3.22)

project(SBCQueensGUI LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(NAME SBCQueensGUIHelpers SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set(CMAKE_CXX_STANDARD 17)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME
  "SBCQueensGUI")


target_link_libraries(${PROJECT_NAME} SBCQueensGUIHelpers::SBCQueensGUIHelpers)