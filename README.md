# DEngine

Essentially a sandbox for learning anything related to 3D simulations and game engine development. The long-term pipedream is to eventually have a lightweight 2D game engine that allows you to do simple game development on Windows and Linux, while also allowing use of the level-editor on Android tablets (and other touch devices). The game engine is primarily built with mobile hardware in mind.

This repository also serves as part of my own portfolio for showing future employers whenever I am job-hunting.

![Screenshot](https://user-images.githubusercontent.com/25203601/134176831-fa3bef12-9c3d-4556-ae7a-a944bb949bce.jpg)

## Highlights

### Powered by Vulkan

Rendering is powered by Vulkan only, and supports rendering GUI as well as the 3D scene. Includes support for multiple 3D viewports and rendering separately to multiple OS windows. Rendering is (almost) completely decoupled from the other modules, i.e GUI or any other logic.

### Custom retained-mode GUI

The GUI module is built from the ground-up to support touch functionality on the same level as keyboard+mouse functionality. It produces a list of API-agnostic data that can be consumed and translated into actual draw calls by the rendering module. It operates on purely data-driven events and does not use any hooks into OS-specific systems. The GUI module does *not* control execution flow of the app. Font rendering is handled by FreeType2.

## How to build

See BUILD.md

## Dependencies

- GLFW3
- Texas
- FreeType2
- Vulkan Memory Allocator

## Showcase

https://user-images.githubusercontent.com/25203601/134176645-5a626e1e-d10b-40cc-910e-e75f7b5f2735.mp4