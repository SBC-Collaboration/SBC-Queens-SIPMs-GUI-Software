# Example usage:
#  mkdir build
#  cd build
#  cmake ../ -G "MinGW Makefiles" -DUSE_VULKAN=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
#  cmake ../ -G "MinGW Makefiles" -DUSE_VULKAN=ON -DCAEN_DIR="X:\\Program Files\\CAEN" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cmake_minimum_required(VERSION 3.11)

project(SiPMControlGUI
  VERSION 0.1.0
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
set(DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/deps 
  CACHE FILEPATH "Directory where all the dependencies are found. Full path only.")
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


# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
# endif()

# There are no C files in this other than coming from dependencies
# C/C++ hard mode: "-Wall"
# C/C++ expert mode: "-Wpedantic" "-Wextra"
#
# Set the project standard and warnings
#

add_compile_options(
  "$<$<CONFIG:DEBUG>:-Og;-ggdb;>"
  "$<$<CONFIG:RELEASE>:-O3;>"
)

# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
# set (CMAKE_CXX_FLAGS "--std=c++17")

# set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -ggdb -Og")
# set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O3")

# CAEN comm library

include(cmake/Dependencies.cmake)
## First VME as it is the main component
if(IS_DIRECTORY ${CAEN_DIR})
  message("-- ${CAEN_DIR} Found!")

  if(LINUX)
    include_directories(SYSTEM ${CAEN_DIR}/CAENVMElib/include)
    link_directories(${CAEN_DIR}/CAENVMElib/lib/x64)
    #target_link_libraries(SiPMControlGUI CAENVMElib)
      ## Then Comm libs
    include_directories(SYSTEM ${CAEN_DIR}/CAENComm/include)
    link_directories(${CAEN_DIR}/CAENComm/lib/x64)

    ## Then digitizers
    include_directories(SYSTEM ${CAEN_DIR}/CAENDigitizer/include)
    link_directories(${CAEN_DIR}/CAENDigitizer/lib/x86_64)
  else()
    include_directories(SYSTEM ${CAEN_DIR}/VME/include)
    link_directories(${CAEN_DIR}/VME/lib/x86_64)
    #target_link_libraries(SiPMControlGUI CAENVMElib)
      ## Then Comm libs
    include_directories(SYSTEM ${CAEN_DIR}/Comm/include)
    link_directories(${CAEN_DIR}/Comm/lib/x86_64)

    ## Then digitizers
    include_directories(SYSTEM ${CAEN_DIR}/Digitizers/Library/include)
    link_directories(${CAEN_DIR}/Digitizers/Library/lib/x86_64)
  endif()

else()
  message(FATAL_ERROR "CAEN not found. Make sure to install VME, Comm and "
    "Digitizers libraries. ${CAEN_DIR}")
endif()

#target_link_libraries(SiPMControlGUI CAENComm)

# my files
include_directories(./include)

file(GLOB sources *.cpp)
file(GLOB srcsources ./src/*.cpp)
file(GLOB renderers ./src/rendering_wrappers/*.cpp)
file(GLOB guis ./src/GUI/*.cpp)
file(GLOB hardware ./src/HardwareHelpers/*.cpp)

add_executable(SiPMControlGUI ${sources} ${srcsources} ${renderers} ${guis} ${hardware})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
else()
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

  if(${PROJECT_NAME}_BUILD_EXECUTABLE AND ${PROJECT_NAME}_ENABLE_UNIT_TESTING)
    target_compile_features(${PROJECT_NAME}_LIB PUBLIC cxx_std_17)
  endif()
endif()
include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++)
endif()

file(GLOB GUI_CONFIG_FILE gui_setup.toml)
file(COPY ${GUI_CONFIG_FILE} DESTINATION ${PROJECT_BINARY_DIR})

find_package(OpenMP REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENMP_LIBRARIES})

# setupapi -> for serial
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARIES} ${IMGUI_LIBRARIES}
  ${ARMADILLO_LIBRARIES}
  sipmcharacterization
  glfw imgui implot
  serial atomic spdlog
  CAENVME
  CAENComm
  CAENDigitizer
)
