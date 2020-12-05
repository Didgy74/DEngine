# DEngine
Essentially a sandbox for learning anything related to 3D simulations and game engine development. The long-term pipedream is to eventually have a lightweight 2D game engine that allows you to do simple game development on Windows and Linux, while also allowing use of the level-editor on Android tablets (and other touch devices). The game engine is primarily built with mobile hardware in mind.

This repository also serves as part of my own portfolio for showing future employers whenever I am job-hunting.

![Screenshot](https://github.com/Didgy74/DEngine/raw/master/Screenshot.jpg)
[Click here](https://github.com/Didgy74/DEngine/raw/master/Usage%20showcase.mp4) for MP4 showcasing the app in action
## Highlights
 ### Powered by Vulkan
Rendering is powered by Vulkan only, and supports rendering GUI as well as the 3D scene. Includes support for multiple 3D viewports and rendering separately to multiple OS windows. Rendering is (almost) completely decoupled from the other modules, i.e GUI or any other logic.

 ### Custom retained-mode GUI
 The GUI module is built from the ground-up to support touch functionality on the same level as keyboard+mouse functionality. It produces a list of API-agnostic data that can be consumed and translated into actual draw calls by the rendering module. It operates on purely data-driven events and does not use any hooks into OS-specific systems. The GUI module does *not* control execution flow of the app. Font rendering is handled by FreeType2.

## How to build
### Windows
Requires:
 - CMake 3.10 or higher

Tested on following compilers:
 - MSVC v142
 - Clang 10
### Android
Requires
 - Device running Android 8.1 or higher
 - Android Studio 4.1
	 From the SDK Manager:
	 - Android SDK Platform 30
	 - Android SDK Build-Tools
	 - Android NDK 21
	 - CMake 3.10
## Dependencies
 - GLFW 3
 - Texas
 - FreeType2

