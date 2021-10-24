#!/bin/bash

# Android Studio stuff
rm android/android.iml
# rm android/local.properties
rm --recursive android/.gradle
rm --recursive android/.idea
rm android/app/app.iml
rm --recursive android/app/build
rm --recursive android/app/out
rm --recursive android/app/.cxx

# VSCode stuff
rm --recursive .vscode
rm --recursive build

# Visual Studio stuff
rm --recursive .vs
rm --recursive out
rm --recursive any/out
rm --recursive desktop/out

# rm CMakeSettings.json

# CLion stuff
rm --recursive .idea
rm --recursive cmake-build-debug
rm --recursive desktop/cmake-build-debug
