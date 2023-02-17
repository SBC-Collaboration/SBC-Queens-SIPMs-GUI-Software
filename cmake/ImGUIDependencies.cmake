include(cmake/CPM.cmake)

# Dear ImGui
# https://github.com/ocornut/imgui
CPMAddPackage(NAME imgui
  VERSION 1.89.2
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

# https://github.com/samhocevar/portable-file-dialogs
CPMAddPackage("gh:samhocevar/portable-file-dialogs#8ccf2a9") # v0.1.0

# https://github.com/pthom/ImFileDialog
# CPMAddPackage(NAME portable-file_dialogs
#   VERSION 0.1.0
#   GITHUB_REPOSITORY samhocevar/portable-file-dialogs)

# https://github.com/mekhontsev/imgui_md
# CPMAddPackage(NAME imgui_md
#   GITHUB_REPOSITORY mekhontsev/imgui_md
#   DOWNLOAD_ONLY YES)
# if(imgui_md_ADDED)
#   	add_library(imgui_md STATIC
#     	${imgui_md_SOURCE_DIR}/imgui_md.cpp
#     )
# 	target_include_directories(imgui_md
#     SYSTEM PUBLIC
#     $<BUILD_INTERFACE:${imgui_md_SOURCE_DIR}>/)
#   	target_link_libraries(imgui_md PUBLIC glfw ${OPENGL_LIBRARIES} ${IMGUI_LIBRARIES})
# endif()