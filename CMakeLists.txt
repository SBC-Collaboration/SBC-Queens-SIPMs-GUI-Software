#
# See sipm_control_gui.sublime-project to see examples on how to compile it.
#

cmake_minimum_required(VERSION 3.11)

include(cmake/CPM.cmake)

project(SBCQueensGUIHelpers
  VERSION 0.2.0
  LANGUAGES C CXX)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

#
# Setup alternative names
#

if(${PROJECT_NAME}_USE_ALT_NAMES)
  string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
  string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
else()
  set(PROJECT_NAME_LOWERCASE ${PROJECT_NAME})
  set(PROJECT_NAME_UPPERCASE ${PROJECT_NAME})
endif()

# Default to debug
set(CAEN_DIR "C:\\Program Files\\CAEN" 
  CACHE FILEPATH "Directory where CAEN VME, Comm, and digitizer files are found")
option(USE_VULKAN OFF)

if(USE_VULKAN)
  add_definitions(-DUSE_VULKAN)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

if (UNIX)
  add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")    #this will allow to use same _DEBUG macro available in both Linux as well as Windows - MSCV environment. Easy to put Debug specific code.
endif (UNIX)


add_compile_options(
  "$<$<CONFIG:DEBUG>:-Og;-ggdb;>"
  "$<$<CONFIG:RELEASE>:-O3;>"
)

# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
# set (CMAKE_CXX_FLAGS "--std=c++17")

# set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -ggdb -Og")
# set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O3")

# CAEN comm library
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.10.0")

include(cmake/Dependencies.cmake)
## First VME as it is the main component
if(IS_DIRECTORY ${CAEN_DIR})
  message("-- ${CAEN_DIR} Found!")

  if(LINUX)
    target_include_directories(serial PUBLIC SYSTEM
    $<BUILD_INTERFACE:${CAEN_DIR}>/include)

    # include_directories(SYSTEM ${CAEN_DIR}/VME/include)
    add_library(CAENVME SHARED IMPORTED GLOBAL)
    set_target_properties(CAENVME PROPERTIES IMPORTED_LOCATION ${CAEN_DIR}/lib/libCAENVME.so)
    add_library(CAENComm SHARED IMPORTED GLOBAL)
    set_target_properties(CAENComm PROPERTIES IMPORTED_LOCATION ${CAEN_DIR}/lib/libCAENComm.so)
    add_library(CAENDigitizer SHARED IMPORTED GLOBAL)
    set_target_properties(CAENDigitizer PROPERTIES IMPORTED_LOCATION ${CAEN_DIR}/lib/libCAENDigitizer.so)
  else()
    target_include_directories(serial PUBLIC SYSTEM
    $<BUILD_INTERFACE:${CAEN_DIR}>/VME/include
    $<BUILD_INTERFACE:${CAEN_DIR}>/Comm/include
    $<BUILD_INTERFACE:${CAEN_DIR}>/Digitizers/Library/include)

    # include_directories(SYSTEM ${CAEN_DIR}/VME/include)
    add_library(CAENVME SHARED IMPORTED GLOBAL)
    set_target_properties(CAENVME PROPERTIES IMPORTED_IMPLIB ${CAEN_DIR}/VME/lib/x86_64/CAENVME.lib)
    add_library(CAENComm SHARED IMPORTED GLOBAL)
    set_target_properties(CAENComm PROPERTIES IMPORTED_IMPLIB ${CAEN_DIR}/Comm/lib/x86_64/CAENComm.lib)
    add_library(CAENDigitizer SHARED IMPORTED GLOBAL)
    set_target_properties(CAENDigitizer PROPERTIES IMPORTED_IMPLIB ${CAEN_DIR}/Digitizers/Library/lib/x86_64/CAENDigitizer.lib)
  endif()

else()
  message(FATAL_ERROR "CAEN not found. Make sure to install VME, Comm and "
    "Digitizers libraries at ${CAEN_DIR}")
endif()

#target_link_libraries(SiPMControlGUI CAENComm)

# my files
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} ${sources} ${headers})

include(cmake/CompilerWarnings.cmake)
set(CMAKE_CXX_STANDARD 17)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_project_warnings(${PROJECT_NAME})

# if(MINGW)
#     set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++)
# endif()

file(GLOB GUI_CONFIG_FILE gui_setup.toml)
file(COPY ${GUI_CONFIG_FILE} DESTINATION ${PROJECT_BINARY_DIR})

# setupapi -> for serial
target_link_libraries(${PROJECT_NAME}
  PUBLIC
  nlohmann_json::nlohmann_json
  imgui
  implot
  readerwriterqueue
  concurrentqueue
  tomlplusplus
  serial
  glfw
  atomic
  CAENVME
  CAENComm
  CAENDigitizer
  Sipmanalysis::Sipmanalysis
  spdlog::spdlog
)


target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION})

# string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)



# packageProject(
#   NAME ${PROJECT_NAME}
#   VERSION ${PROJECT_VERSION}
#   NAMESPACE ${PROJECT_NAME}
#   BINARY_DIR ${PROJECT_BINARY_DIR}
#   INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
#   INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
#   VERSION_HEADER "${VERSION_HEADER_LOCATION}"
#   COMPATIBILITY SameMajorVersion
#   DEPENDENCIES "spdlog;Armadillo;nlohmann_json;imgui;implot;tomlplusplus;serial"
# )
