# DEngine

Game engine project by Didgy (Nils Petter Skålerud)

How to build:

Windows:
The project can be built as-is on MSVC and MinGW using CMake

Linux:
Should just be a matter of doing cmake ./ and then make in terminal. Will write detailed 

Ubuntu packages required to build:
libglew-dev
libglfw3-dev

NOTE! 32-bit is not supported.

How to install:
git clone https://github.com/Didgy74/DEngine.git
git submodule init
git submodule update
cmake ./
make


The engine uses the following libraries:
* GLEW
* GLFW
* OpenGL
* nlohmann
* fx-gltf
* stb_image
