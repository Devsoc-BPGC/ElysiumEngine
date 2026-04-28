# ElysiumEngine

A lightweight 2D physics engine and game framework built with C++20 and SFML.

## Features

- **Physics Engine**: Supports Rigid Body dynamics, AABB collision detection, and Sphere/Box colliders.
- **Component System**: Simple GameObject-Component architecture for easy extensibility.
- **SFML Integration**: Built-in renderer using SFML 3 for high-performance 2D graphics.

## Project Structure

- `app/`: Application entry point (`main.cpp`).
- `engine/`: Core engine logic.
    - `core/`: GameObject, Scene, and Component systems.
    - `physics/`: Rigid body dynamics, colliders, and collision detection.
- `external/`: Third-party dependencies (SFML as a submodule).

## Prerequisites

- **C++20 Compiler**: GCC 10+, Clang 10+, or MSVC 2019+.
- **CMake**: Version 3.15 or higher.
- **Dependencies**: SFML dependencies (X11, OpenGL, etc. on Linux).

## Building the Project

1. **Clone the repository with submodules**:
   ```bash
   git clone --recursive https://github.com/yourusername/ElysiumEngine.git
   cd ElysiumEngine
   ```
   *If you already cloned it without submodules, run:*
   ```bash
   git submodule update --init --recursive
   ```

2. **Configure and Build**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run the Application**:
   ```bash
   ./ElysiumApp
   ```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
