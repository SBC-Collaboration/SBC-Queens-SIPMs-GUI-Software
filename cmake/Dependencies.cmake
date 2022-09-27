set(DEPENDENCY_DIR ${CMAKE_SOURCE_DIR}/deps
  CACHE FILEPATH "Directory where all the dependencies are found.
  Full path only.")

# OpenGL
find_package( OpenGL REQUIRED )

# What is SYSTEM?
# From CMAKE documentation
# If the SYSTEM option is given, the compiler will be told the directories are
# meant as system include directories on some platforms. Signalling this
# setting might achieve effects such as the compiler skipping warnings, or
# these fixed-install system files not being considered in dependency
# calculations - see compiler docs.
include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )

# GL3W
set(GL3W_DIR ${DEPENDENCY_DIR}/gl3w)
if(IS_DIRECTORY ${GL3W_DIR})
  add_subdirectory(${GL3W_DIR} binary_dir/gl3w EXCLUDE_FROM_ALL)
  include_directories(SYSTEM ${GL3W_DIR}/include)
else()
  message(FATAL_ERROR "GL3W library not found. "
    "Make sure to run git submodules init first")
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
  include_directories(SYSTEM ${GLFW_DIR}/include)
  set(IMGUI_LIBRARIES "glfw")
else()
  message(FATAL_ERROR "GLFW not found. "
    "Make sure to run git submodules init first")
endif()

# Libraries
if(USE_VULKAN)
  find_package(Vulkan REQUIRED)
  set(IMGUI_LIBRARIES "${IMGUI_LIBRARIES};Vulkan::Vulkan")
  # include_directories( ${VULKAN_INCLUDE_DIR} )
  include_directories(SYSTEM ${GLFW_DIR}/deps )
endif()

# Dear ImGui
set(IMGUI_DIR ${DEPENDENCY_DIR}/imgui)
if(IS_DIRECTORY ${IMGUI_DIR})
  include_directories(SYSTEM ${IMGUI_DIR} ${IMGUI_DIR}/backends)

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
  target_link_libraries(imgui PRIVATE ${OPENGL_LIBRARIES} ${IMGUI_LIBRARIES})
else()
  message(FATAL_ERROR "ImGUI not found.
    Make sure to run git submodules init first")
endif()

# Implot
set(IMPLOT_DIR ${DEPENDENCY_DIR}/implot)
if(IS_DIRECTORY ${IMPLOT_DIR})
  include_directories(SYSTEM ${IMPLOT_DIR})
  add_library(implot STATIC
      ${IMPLOT_DIR}/implot_items.cpp
      ${IMPLOT_DIR}/implot.cpp
      ${IMPLOT_DIR}/implot_demo.cpp)
else()
  message(FATAL_ERROR "ImPlot not found.
    Make sure to run git submodules init first")
endif()

# serial
# https://github.com/wjwwood/serial
# Do not add as add_subdirectory, it will not work
set(SERIAL_DIR ${DEPENDENCY_DIR}//serial)
if(IS_DIRECTORY ${SERIAL_DIR})
  include_directories(SYSTEM ${SERIAL_DIR}/include)
  add_library(serial STATIC
    ${SERIAL_DIR}/src/serial.cc
    ${SERIAL_DIR}/src/impl/win.cc
    ${SERIAL_DIR}/src/impl/unix.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_win.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_linux.cc
    ${SERIAL_DIR}/src/impl/list_ports/list_ports_osx.cc)
  if(LINUX)
    target_link_libraries(serial PRIVATE )
  else()
    target_link_libraries(serial PRIVATE setupapi)
  endif()

else()
  message(FATAL_ERROR "Serial library not found.
    Make sure to run git submodules init first")
endif()

# Use vulkan/opengl32 headers from glfw:
# include_directories(${GLFW_DIR}/deps)

# json serializer
# https://github.com/nlohmann/json
set(JSON_DIR ${DEPENDENCY_DIR}/json)
if(IS_DIRECTORY ${JSON_DIR})
  include_directories(SYSTEM ${JSON_DIR}/single_include)
else()
  message(FATAL_ERROR "json single_include not found.
    Make sure to run git submodules init first")
endif()

# spdlog
# https://github.com/gabime/spdlog
set(SPDLOG_DIR ${DEPENDENCY_DIR}/spdlog)
if(IS_DIRECTORY ${SPDLOG_DIR})
  add_subdirectory(${SPDLOG_DIR} binary_dir/spdlog EXCLUDE_FROM_ALL)
  include_directories(SYSTEM ${SPDLOG_DIR}/include)
else()
  message(FATAL_ERROR "spdlog not found.
    Make sure to run git submodules init first")
endif()

# boost SML
# set(BOOST_SML_DIR ./deps/sml)
# include_directories(${BOOST_SML_DIR}/include)

# readerwriterqueue
set (RWQUEUE_DIR ${DEPENDENCY_DIR}/readerwriterqueue)
if(IS_DIRECTORY ${RWQUEUE_DIR})
  include_directories(SYSTEM ${RWQUEUE_DIR})
else()
  message(FATAL_ERROR "readerwriterqueue not found.
    Make sure to run git submodules init first")
endif()

# # concurrentqueue
# # its a multi-user of the above library
set (CONCURRENTQ_DIR ${DEPENDENCY_DIR}/concurrentqueue)
if(IS_DIRECTORY ${CONCURRENTQ_DIR})
  include_directories(SYSTEM ${CONCURRENTQ_DIR})
else()
  message(FATAL_ERROR "concurrentqueue not found.
    Make sure to run git submodules init first")
endif()

set (TOML_DIR ${DEPENDENCY_DIR}/tomlplusplus)
if(IS_DIRECTORY ${TOML_DIR})
  include_directories(SYSTEM ${TOML_DIR})
else()
  message(FATAL_ERROR "TOML++ not found.
    Make sure to run git submodules init first")
endif()

find_package(Armadillo 11.2.3 REQUIRED)
include_directories(SYSTEM "${ARMADILLO_INCLUDE_DIR}")

set (SIPM_DIR /home/sbc-queens-linux/tmp)
if(IS_DIRECTORY ${SIPM_DIR})
  link_directories(${SIPM_DIR}/lib)
  include_directories(SYSTEM ${SIPM_DIR}/include)
else()
  message(FATAL_ERROR "${SIPM_DIR} not found.
    Make sure to run git submodules init first")
endif()