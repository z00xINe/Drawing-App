# Drawing App

A feature-rich 2D Drawing Application built with C++ and Windows GDI+ API. This desktop application demonstrates fundamental computer graphics algorithms and provides interactive tools for creating and manipulating vector shapes.

## Overview

This project implements various classic computer graphics algorithms for rendering circles, ellipses, and curves. It serves as both a practical drawing tool and an educational resource for understanding low-level graphics programming on Windows platforms.

## Features

- Multiple circle drawing algorithms:
  - Direct method
  - Polar coordinates method
  - Iterative polar method
  - Midpoint algorithm
  - Modified midpoint algorithm
- Ellipse rendering using Direct, Polar, and Midpoint techniques
- Cardinal spline curve drawing with up to 4 control points
- Hermite curve filling for square regions
- Quarter-section filling with circles or radial lines
- Custom crosshair cursor rendering
- Color selection (Red, Green, Blue, Black)
- White background toggle
- Save canvas to PNG file
- Load previously saved shapes from file
- Clear canvas functionality
- Real-time mouse interaction and rendering

## Technologies Used

- C++ (C++20 standard)
- Windows API (Win32)
- GDI+ for graphics rendering
- CMake for build configuration

## Prerequisites

- Windows operating system
- CMake 3.29 or higher
- C++ compiler with C++20 support (MSVC recommended)
- GDI+ libraries (included with Windows SDK)

## Building the Project

1. Clone the repository:
```bash
git clone https://github.com/z00xINe/Drawing-App.git
cd Drawing-App
```
2. Generate build files with CMake:
```bash
mkdir build
cd build
cmake ..
```
3. Build the project:
```bash
cmake --build .
```
4. Run the executable:
```bash
./Project.exe
```

## Usage

- Left-click on the canvas to place shapes or control points
- Use the Options menu to select drawing algorithms, colors, and tools
- Select "Draw Cardinal Spline" then click 4 points to render a smooth curve
- Choose circle algorithms before clicking to draw circles with specific methods
- Use "Save shapes to file" to export current drawing as PNG
- Use "Load shapes from file" to restore previously saved work

## Project Structure

- main.cpp - Application entry point, window procedure, and event handling
- draw.h - Core drawing utilities
- circle.h - Circle drawing algorithm implementations
- ellipse.h - Ellipse rendering functions
- hermite.h - Hermite curve generation logic
- CMakeLists.txt - CMake build configuration
- myshapes.txt - Example shape data file

## License

This project is open source and available under the MIT License.

## Authors

- Youssef Mohamed (@z00xINe)
- Mostafa Ibrahim (@darch244)
- Zeiad Mohamed (@khaledmoabdo)
