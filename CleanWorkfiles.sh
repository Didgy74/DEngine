#!/bin/bash

# Android Studio stuff
rm android/android.iml
rm android/local.properties
rm -r android/.gradle
rm -r android/.idea
rm android/app/app.iml
rm -r android/app/build
rm -r android/app/.cxx

# VSCode stuff
rm -r .vscode
rm -r build

# Visual Studio stuff
rm -r .vs
rm -r out
# rm CMakeSettings.json