# Realtime Rope Simulation

This is a real-time 3D rope simulation project built using C++ and OpenGL. The physics engine is powered by **Extended Position Based Dynamics (XPBD)**, allowing for stable, robust, and interactive simulation of a rope with various constraints.

## Features

*   **XPBD Physics:** Stable rope simulation with configurable iterations (substeps), damping, bounciness (restitution), and compliance.
*   **Interactive Controls:** 
    *   **Mouse Interaction:** Left-click and drag to grab and move rope particles in 3D space in real-time.
    *   **Camera Controls:** Right-click and drag to rotate the camera. Use `W`, `A`, `S`, `D` to fly around the scene.
*   **ImGui Integration:** A real-time UI allows you to tweak simulation parameters on the fly:
    *   Number of rope particles
    *   Rest length and tube visual radius/sides
    *   Physics timescale
    *   Enable/disable gravity
    *   Toggle rendering of environment objects (Floor, Sphere, Box)
    *   Toggle "Pin Last" particle.
*   **3D Mesh Generation:** The rope is rendered as a dynamic 3D tube mesh constructed from discrete particle positions.
*   **Collision Objects:** The scene includes primitive bounding objects (Floor, Sphere, Box) that the rope can potentially interact with.

## Dependencies

*   **OpenGL 3.3+**
*   **GLFW** for window creation and input handling.
*   **GLAD** for OpenGL function pointers.
*   **GLM** for mathematics (vectors, matrices).
*   **Dear ImGui** for the user interface.

## Building the Project

This project uses **CMake** for its build system.

1.  Ensure you have a modern C++ compiler, CMake (3.16+), and OpenGL development libraries installed.
2.  Clone the repository or open the project folder.
3.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```
4.  Generate the build files with CMake:
    ```bash
    cmake ..
    ```
5.  Build the project:
    ```bash
    cmake --build .
    ```

## Usage

After building the application, you can run the executable `app`.

*   **Move Camera:** Hold **Right Mouse Button** and drag. Use **W, A, S, D** to move forward, left, backward, and right.
*   **Interact with Rope:** Point at a rope particle, hold **Left Mouse Button**, and drag to pull it around.
*   **Tweak Settings:** Use the "Rope Params" ImGui window on the screen to adjust simulation attributes dynamically.

## License

This project was developed as a final project for a Realtime Computer Graphics course.
