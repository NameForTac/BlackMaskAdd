# ImageMaskTool (C++ / Qt 6)

## Introduction
A tool to batch add masks to images, supporting World of Warcraft BLP format.

## Prerequisites
1.  **C++ Compiler**: Visual Studio 2022 (MSVC) or MinGW.
2.  **CMake**: Version 3.16 or higher.
3.  **Qt 6**: Core, Gui, Widgets, Concurrent modules.

## How to Build
1.  Ensure you have properly installed Qt 6 and Visual Studio (with C++ CMake tools).
2.  Double-click `build_and_run.bat`.

## Troubleshooting
If `cmake` fails to find Qt6:
1.  Locate your Qt installation (e.g., `C:\Qt\6.7.0\msvc2019_64`).
2.  Open `build_and_run.bat`.
3.  Modify the cmake line to: `cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"` (replace with your actual path).
