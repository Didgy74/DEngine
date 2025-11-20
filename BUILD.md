Git command for Didgy:
```
git clone --recursive --branch didgy-dev --shallow-submodules --single-branch --depth 1 git@github.com:Didgy74/DEngine.git
```

# Build instructions
This document contains instructions for building this project on Windows, Linux and Android.

# Requirements
This is a list of requirements you will need to be able to build this project. The project is expected to work on newer versions of the required software, but is not guaranteed.

In general:
- A Windows or Linux host machine. 
  - is not possible to compile the project from Android itself. macOS is not supported.
- CMake
- A C++20 compatible compiler
- PowerShell
  - To run the prebuild step of bunding assets
- GLSLC
  - For precompiling shaders
  - Download from LunarG Vulkan SDK
  - TODO: I could check precompiled shaders into source control at some point, so one may work on the parts of the project that don't require assets or shaders.

### Windows
The following compilers have been tested and proven to work:
 - MSVC x64 14.29
 - Clang x64 11.0

### Linux
Required packages:
- glfw3-wayland **or** glfw3-x11
- libfreetype2
- libzstd

The following compilers have been tested and proven to work:
 - MSVC x64 14.29
 - Clang x64 11.0

### Android
- Device running 64-bit Android 14

# Downloading the project with Git
Open a terminal in the folder you want the project stored in. 

**Windows:** Download Git for Windows and right click the folder -> Open in Git Bash

Use Git to download the repository by running the following in a terminal:
```
git clone https://github.com/Didgy74/DEngine.git
cd DEngine
git submodule update --init --recursive
```
This will download the repository and all the submodule dependencies.

# Windows
Just open the thing in Visual Studio.

Will add more instructions later.

# Linux
Will add more instructions later.

# Android
In Android Studio, open the folder `DEngine/android` and press build.
