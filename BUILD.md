Git command for Didgy:
```
git clone --recursive --branch didgy-dev --shallow-submodules --single-branch --depth 1 git@github.com:Didgy74/DEngine.git
```

# Build instructions
This document contains instructions for building this project on Windows, Linux and Android.

# Requirements
This is a list of requirements you will need to be able to build this project. The project is expected to work on newer versions of the required software, but is not guaranteed.

In general:
- A Window or Linux host machine.
	It is not possible to compile the project from Android itself.
- CMake v3.19
- A C++20 compatible compiler

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
- Device running 64-bit Android 8.1
- Android Studio v4.2
  - SDK Platform v30 (Android 11.0)
  - SDK Build Tools v30.0.3
  - NDK 23.0.7272597
 
The following compilers have been tested and proven to work:
- Clang ARMv8 64-bit 11.0
  Included with the NDK.
- Clang ARMv7 32-bit 11.0
  Included with the NDK.


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

In Android Studio, if the CMake version available is lower than 3.19 you will need to manually download CMake 3.19 or newer. In the file `DEngine/android/local.properties`` add the path to your CMake program folder like so:
```
# Path to CMake program folder  
cmake.dir=Path\\To\\CMake
```
Windows example:
```
# Path to CMake program folder  
cmake.dir=C\:\\Program Files\\CMake
```
Linux example:
```
TODO
```

Note: It is supposedly possible to simply add the CMake path to the system `PATH` environment variable and have Android Studio automatically pick it up, but this has not always proved succesful.
