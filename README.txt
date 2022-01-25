If compiling this for windows make sure you have downloaded and installed:

* MSYS2 (with CMAKE, build-essentials, clang-tools-extra PREFERRED). See below for further instructions
* (optional) Git for windows (to git clone the dependencies if not included)

These are all the dependencies so far:

* gl3w				- https://github.com/skaslev/gl3w
* glfw				- https://github.com/glfw/glfw
* imgui				- https://github.com/ocornut/imgui
* implot			- https://github.com/epezent/implot
* json				- https://github.com/nlohmann/json
* serial 			- https://github.com/wjwwood/serial
* spglog 			- https://github.com/gabime/spdlog
* readerwriterqueue - https://github.com/cameron314/readerwriterqueue
* concurrentqueue 	- https://github.com/cameron314/concurrentqueue
* tomlcplusplus 	- https://github.com/marzer/tomlplusplus

The compilation steps are:

mkdir build
cd build
cmake ../ -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
mingw32-make -j8



You will need to install mingw64 using MSYS2 first and install the toolchain and extras (for clang-tidy and clangd)

To install MSYS2 follow:

	https://www.msys2.org/

Then install cmake using msys2:

	https://www.msys2.org/docs/cmake/
	or
	pacman -S mingw-w64-x86_64-cmake
	into a MSYS terminal

If the intention is to develop the code:
After installing MSYS2, you will need clang-tidy and clangd follow. For that, follow:

	https://packages.msys2.org/package/mingw-w64-x86_64-clang-tools-extra

Then, follow this link to install LSP:
	https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/sublime_ide.md#Setup
