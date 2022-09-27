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


# add_compile_options(
#   "-Wall"
#   "$<$<CONFIG:DEBUG>:-Og;-ggdb;--std=c++17>"
#   "$<$<CONFIG:RELEASE>:-O3;--std=c++17;>"
# )

# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
# set (CMAKE_CXX_FLAGS "--std=c++17")

# set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -ggdb -Og")
# set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O3")

# OpenGL
find_package( OpenGL REQUIRED )
include_directories( ${OPENGL_INCLUDE_DIR} )

# GL3W
set(GL3W_DIR ${DEPENDENCY_DIR}/gl3w) 
if(IS_DIRECTORY ${GL3W_DIR})
  add_subdirectory(${GL3W_DIR} binary_dir/gl3w EXCLUDE_FROM_ALL)
  include_directories(${GL3W_DIR}/include)
else()
  message(FATAL_ERROR "GL3W library not found. Make sure to run git submodules init first")
endif()

# GLFW
set(GLFW_DIR ${DEPENDENCY_DIR}/glfw) # Set this to point to an up-to-date GLFW repo
if(IS_DIRECTORY ${GLFW_DIR})
  option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
  option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
  option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
  option(GLFW_INSTALL "Generate installation target" OFF)
  option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
  add_subdirectory(${GLFW_DIR} binary_dir/glfw EXCLUDE_FROM_ALL)
  include_directories(${GLFW_DIR}/include )
  set(IMGUI_LIBRARIES "glfw")
else()
  message(FATAL_ERROR "GLFW not found. Make sure to run git submodules init first")
endif()

# Libraries
if(USE_VULKAN)
  find_package(Vulkan REQUIRED)
  set(IMGUI_LIBRARIES "${IMGUI_LIBRARIES};Vulkan::Vulkan")
  # include_directories( ${VULKAN_INCLUDE_DIR} )
  include_directories( ${GLFW_DIR}/deps )
endif()

# Dear ImGui
set(IMGUI_DIR ${DEPENDENCY_DIR}/imgui)
if(IS_DIRECTORY ${IMGUI_DIR})
  include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

  list(APPEND IMGUI_BACKENDS ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
  if(USE_VULKAN)
    list(APPEND IMGUI_BACKENDS ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
  else()
    list(APPEND IMGUI_BACKENDS ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
  endif()

  add_library(imgui STATIC
    
    ${IMGUI_BACKENDS}
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp)
  target_link_libraries(imgui ${OPENGL_LIBRARIES} ${IMGUI_LIBRARIES})
else()
  message(FATAL_ERROR "ImGUI not found. Make sure to run git submodules init first")
endif()

# Implot
set(IMPLOT_DIR ${DEPENDENCY_DIR}/implot)
if(IS_DIRECTORY ${IMPLOT_DIR})
  include_directories(${IMPLOT_DIR})
  add_library(implot STATIC
      ${IMPLOT_DIR}/implot_items.cpp
      ${IMPLOT_DIR}/implot.cpp
      ${IMPLOT_DIR}/implot_demo.cpp)
else()
  message(FATAL_ERROR "ImPlot not found. Make sure to run git submodules init first")
endif()

# serial
# https://github.com/wjwwood/serial
# Do not add as add_subdirectory, it will not work
set(SERIAL_DIR ${DEPENDENCY_DIR}//serial) 
if(IS_DIRECTORY ${SERIAL_DIR})
  include_directories(${SERIAL_DIR}/include)
  add_library(serial STATIC  
    ${SERIAL_DIR}/src/serial.cc
    ${SERIAL_DIR}/src/impl/win.cc
    ${SERIAL_DIR}/src/impl/unix.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_win.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_linux.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_osx.cc)
  if(LINUX)
    target_link_libraries(serial)
  else()
    target_link_libraries(serial setupapi)
  endif()

else()
  message(FATAL_ERROR "Serial library not found. Make sure to run git submodules init first")
endif()

# Use vulkan/opengl32 headers from glfw:
# include_directories(${GLFW_DIR}/deps)

# json serializer
# https://github.com/nlohmann/json
set(JSON_DIR ${DEPENDENCY_DIR}/json) 
if(IS_DIRECTORY ${JSON_DIR})
  include_directories(${JSON_DIR}/single_include)
else()
  message(FATAL_ERROR "json single_include not found. Make sure to run git submodules init first")
endif()

# spdlog
# https://github.com/gabime/spdlog
set(SPDLOG_DIR ${DEPENDENCY_DIR}/spdlog)
if(IS_DIRECTORY ${SPDLOG_DIR})
  add_subdirectory(${SPDLOG_DIR} binary_dir/spdlog EXCLUDE_FROM_ALL)
  include_directories(${SPDLOG_DIR}/include)
else()
  message(FATAL_ERROR "spdlog not found. Make sure to run git submodules init first")
endif()

# boost SML
# set(BOOST_SML_DIR ./deps/sml)
# include_directories(${BOOST_SML_DIR}/include)

# readerwriterqueue
set (RWQUEUE_DIR ${DEPENDENCY_DIR}/readerwriterqueue)
if(IS_DIRECTORY ${RWQUEUE_DIR})
  include_directories(${RWQUEUE_DIR})
else()
  message(FATAL_ERROR "readerwriterqueue not found. Make sure to run git submodules init first")
endif()

# # concurrentqueue
# # its a multi-user of the above library
set (CONCURRENTQ_DIR ${DEPENDENCY_DIR}/concurrentqueue)
if(IS_DIRECTORY ${CONCURRENTQ_DIR})
  include_directories(${CONCURRENTQ_DIR})
else()
  message(FATAL_ERROR "concurrentqueue not found. Make sure to run git submodules init first")
endif()

set (TOML_DIR ${DEPENDENCY_DIR}/tomlplusplus)
if(IS_DIRECTORY ${TOML_DIR})
  include_directories(${TOML_DIR})
else()
  message(FATAL_ERROR "TOML++ not found. Make sure to run git submodules init first")
endif()

find_package(Armadillo 11.2.3 REQUIRED)
include_directories("${ARMADILLO_INCLUDE_DIR}")
# target_link_libraries(${PROJECT_NAME} PUBLIC ${ARMADILLO_LIBRARIES})

# CAEN comm library

## First VME as it is the main component
if(IS_DIRECTORY ${CAEN_DIR})
  message("-- ${CAEN_DIR} Found!")

  if(LINUX)
    include_directories(${CAEN_DIR}/CAENVMElib/include)
    link_directories(${CAEN_DIR}/CAENVMElib/lib/x64)
    #target_link_libraries(SiPMControlGUI CAENVMElib)
      ## Then Comm libs
    include_directories(${CAEN_DIR}/CAENComm/include)
    link_directories(${CAEN_DIR}/CAENComm/lib/x64)

    ## Then digitizers
    include_directories(${CAEN_DIR}/CAENDigitizer/include)
    link_directories(${CAEN_DIR}/CAENDigitizer/lib/x86_64)
  else()
    include_directories(${CAEN_DIR}/VME/include)
    link_directories(${CAEN_DIR}/VME/lib/x86_64)
    #target_link_libraries(SiPMControlGUI CAENVMElib)
      ## Then Comm libs
    include_directories(${CAEN_DIR}/Comm/include)
    link_directories(${CAEN_DIR}/Comm/lib/x86_64)

    ## Then digitizers
    include_directories(${CAEN_DIR}/Digitizers/Library/include)
    link_directories(${CAEN_DIR}/Digitizers/Library/lib/x86_64)
  endif()

else()
  message(FATAL_ERROR "CAEN not found. Make sure to install VME, Comm and Digitizers libraries. ${CAEN_DIR}")
endif()
#target_link_libraries(SiPMControlGUI CAENComm)

# my files
include_directories(./include)

file(GLOB sources *.cpp)
file(GLOB srcsources ./src/*.cpp)
file(GLOB renderers ./src/rendering_wrappers/*.cpp)
file(GLOB guis ./src/GUI/*.cpp)

add_executable(SiPMControlGUI ${sources} ${srcsources} ${renderers} ${guis})

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
# set_project_warnings(${PROJECT_NAME})

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++)
endif()

file(GLOB GUI_CONFIG_FILE gui_setup.toml)
file(COPY ${GUI_CONFIG_FILE} DESTINATION ${PROJECT_BINARY_DIR})

# setupapi -> for serial
target_link_libraries(SiPMControlGUI ${LIBRARIES} ${IMGUI_LIBRARIES}
  ${ARMADILLO_LIBRARIES}
  glfw imgui implot
  serial atomic spdlog
  CAENVME
  CAENComm
  CAENDigitizer)
