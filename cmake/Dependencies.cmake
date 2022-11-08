set(DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/deps
  CACHE FILEPATH "Directory where all the dependencies are found.
  Full path only.")

include(cmake/CPM.cmake)

# OpenGL
find_package(OpenGL REQUIRED)
find_package(OpenMP REQUIRED)

# What is SYSTEM?
# From CMAKE documentation
# If the SYSTEM option is given, the compiler will be told the directories are
# meant as system include directories on some platforms. Signalling this
# setting might achieve effects such as the compiler skipping warnings, or
# these fixed-install system files not being considered in dependency
# calculations - see compiler docs.
include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})

# GL3W
CPMAddPackage("gh:skaslev/gl3w#5f8d7fd")

# GLFW
CPMAddPackage(NAME glfw
  GIT_TAG dd8a678
  GITHUB_REPOSITORY glfw/glfw
  OPTIONS "GLFW_BUILD_EXAMPLES OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_DOCS OFF"
  "GLFW_INSTALL OFF" "GLFW_DOCUMENT_INTERNALS OFF")

# Libraries
if(USE_VULKAN)
  find_package(Vulkan REQUIRED)
  set(IMGUI_LIBRARIES "${IMGUI_LIBRARIES};Vulkan::Vulkan")
  # include_directories( ${VULKAN_INCLUDE_DIR} )
  include_directories(SYSTEM ${GLFW_DIR}/deps)
endif()

# Dear ImGui
# https://github.com/ocornut/imgui
CPMAddPackage(NAME imgui
  VERSION 1.88
  GITHUB_REPOSITORY ocornut/imgui
  DOWNLOAD_ONLY YES)

if(imgui_ADDED)
  list(APPEND ImGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

  if(USE_VULKAN)
    list(APPEND ImGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
  else()
    list(APPEND ImGUI_BACKENDS ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  endif()

  add_library(imgui STATIC
    ${ImGUI_BACKENDS}
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)

  target_include_directories(imgui
    SYSTEM PUBLIC
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>/
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>/backends)
  target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES} ${IMGUI_LIBRARIES})
else()
  message(FATAL_ERROR "ImGUI not found.")
endif()

# Implot
CPMAddPackage(NAME implot
  VERSION 0.14
  GITHUB_REPOSITORY epezent/implot
  DOWNLOAD_ONLY YES)

if(implot_ADDED)
  add_library(implot STATIC
      ${implot_SOURCE_DIR}/implot_items.cpp
      ${implot_SOURCE_DIR}/implot.cpp
      ${implot_SOURCE_DIR}/implot_demo.cpp)

  target_include_directories(implot SYSTEM PUBLIC
    imgui $<BUILD_INTERFACE:${implot_SOURCE_DIR}>/)
  target_link_libraries(implot PRIVATE imgui)
else()
  message(FATAL_ERROR "ImPlot not found.
    Make sure to run git submodules init first")
endif()

# serial
# https://github.com/wjwwood/serial
# Do not add as add_subdirectory, it will not work
CPMAddPackage(NAME serial
  GIT_TAG 1.2.1
  GITHUB_REPOSITORY wjwwood/serial
  DOWNLOAD_ONLY YES)

if(serial_ADDED)
  add_library(serial STATIC
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/serial.cc
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/impl/win.cc
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/impl/unix.cc
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/impl/list_ports/list_ports_win.cc
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/impl/list_ports/list_ports_linux.cc
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/src/impl/list_ports/list_ports_osx.cc)
  if(LINUX)
    target_link_libraries(serial PRIVATE)
  else()
    target_link_libraries(serial PRIVATE setupapi)
  endif()
  target_include_directories(serial PUBLIC SYSTEM
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${serial_SOURCE_DIR}>/include)
endif()

# Use vulkan/opengl32 headers from glfw:
# include_directories(${GLFW_DIR}/deps)

# json serializer
# https://github.com/nlohmann/json
CPMAddPackage("gh:nlohmann/json@3.10.5")


# spdlog
# https://github.com/gabime/spdlog
CPMAddPackage("gh:gabime/spdlog@1.8.2")

# readerwriterqueue
CPMAddPackage("gh:cameron314/readerwriterqueue@1.0.6")

# if(IS_DIRECTORY ${RWQUEUE_DIR})
#   include_directories(SYSTEM ${RWQUEUE_DIR})
# else()
#   message(FATAL_ERROR "readerwriterqueue not found.
#     Make sure to run git submodules init first")
# endif()

# # concurrentqueue
# # its a multi-user of the above library
CPMAddPackage("gh:cameron314/concurrentqueue@1.0.3")
# set (CONCURRENTQ_DIR ${DEPENDENCY_DIR}/concurrentqueue)
# if(IS_DIRECTORY ${CONCURRENTQ_DIR})
#   include_directories(SYSTEM ${CONCURRENTQ_DIR})
# else()
#   message(FATAL_ERROR "concurrentqueue not found.
#     Make sure to run git submodules init first")
# endif()

CPMAddPackage(NAME tomlplusplus
  VERSION 3.2.0
  GITHUB_REPOSITORY marzer/tomlplusplus
  DOWNLOAD_ONLY YES)
if(tomlplusplus_ADDED)
  add_library(tomlplusplus INTERFACE IMPORTED)
  target_include_directories(tomlplusplus SYSTEM INTERFACE
    $<BUILD_INTERFACE:${tomlplusplus_SOURCE_DIR}>/)
endif()
# CPMAddPackage(NAME readerwriterqueue
#   VERSION 1.0.6
#   GITHUB_REPOSITORY cameron314/readerwriterqueue
#   DOWNLOAD_ONLY YES)
# if(concurrentqueue_ADDED)
#   add_library(readerwriterqueue INTERFACE IMPORTED)
#   target_include_directories(readerwriterqueue SYSTEM INTERFACE
#     $<BUILD_INTERFACE:${readerwriterqueue_SOURCE_DIR}>/)
# endif()
# # if(IS_DIRECTORY ${RWQUEUE_DIR})
# #   include_directories(SYSTEM ${RWQUEUE_DIR})
# # else()
# #   message(FATAL_ERROR "readerwriterqueue not found.
# #     Make sure to run git submodules init first")
# # endif()

# # # concurrentqueue
# # # its a multi-user of the above library
# CPMAddPackage(NAME concurrentqueue
#   VERSION 1.0.3
#   GITHUB_REPOSITORY cameron314/concurrentqueue
#   DOWNLOAD_ONLY YES)
# if(concurrentqueue_ADDED)
#   add_library(concurrentqueue INTERFACE IMPORTED)
#   target_include_directories(concurrentqueue SYSTEM INTERFACE
#     $<BUILD_INTERFACE:${concurrentqueue_SOURCE_DIR}>/)
# endif()
# # set (CONCURRENTQ_DIR ${DEPENDENCY_DIR}/concurrentqueue)
# # if(IS_DIRECTORY ${CONCURRENTQ_DIR})
# #   include_directories(SYSTEM ${CONCURRENTQ_DIR})
# # else()
# #   message(FATAL_ERROR "concurrentqueue not found.
# #     Make sure to run git submodules init first")
# # endif()


# set(TOML_DIR ${DEPENDENCY_DIR}/tomlplusplus)
# if(IS_DIRECTORY ${TOML_DIR})
#   include_directories(SYSTEM ${TOML_DIR})
# else()
#   message(FATAL_ERROR "TOML++ not found.
#     Make sure to run git submodules init first")
# endif()

# find_package(Armadillo 11.2.3 REQUIRED)
# CPMAddPackage(
#   NAME ensmallen
#   GIT_TAG  2.19.0
#   GITHUB_REPOSITORY mlpack/ensmallen
#   DOWNLOAD_ONLY YES
# )
# if(ensmallen_ADDED)
#   add_library(ensmallen INTERFACE IMPORTED)
#   target_link_libraries(ensmallen INTERFACE Armadillo)
#   target_include_directories(ensmallen
#     SYSTEM INTERFACE
#     $<INSTALL_INTERFACE:include>
#     $<BUILD_INTERFACE:${ensmallen_SOURCE_DIR}>/include)
# endif()

# set (SIPM_DIR /home/sbc-queens-linux/tmp/)
CPMAddPackage(NAME Sipmanalysis SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../SiPMCharacteriazation)
# set (SIPM_DIR ${CMAKE_SOURCE_DIR}/../SiPMCharacteriazation/tmp/)
# if(IS_DIRECTORY ${SIPM_DIR})
#   link_directories(${SIPM_DIR}/lib)
#   include_directories(SYSTEM ${SIPM_DIR}/include/Sipmanalysis-0.1/)
# else()
#   message(FATAL_ERROR "${SIPM_DIR} not found.
#     Make sure to run git submodules init first")
# endif()