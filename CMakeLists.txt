cmake_minimum_required(VERSION 3.16)
project(RopeSimulation)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_STANDARD 17)

add_executable(app src/main.cpp lib/glad.c)

target_include_directories(app PRIVATE include)

target_link_libraries(app
    ${CMAKE_SOURCE_DIR}/lib/libglfw3.a
    opengl32
    gdi32
    user32
)