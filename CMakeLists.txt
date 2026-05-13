cmake_minimum_required(VERSION 3.16)
project(RopeSimulation)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_STANDARD 17)

add_executable(app 
    src/main.cpp 
    lib/glad.c
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_impl_glfw.cpp
    lib/imgui/imgui_impl_opengl3.cpp
    lib/imgui/imstb_rectpack.h
    lib/imgui/imstb_textedit.h
    lib/imgui/imstb_truetype.h
    lib/imgui/imgui_impl_opengl3_loader.h
)

target_include_directories(app PRIVATE 
    include
    lib/imgui
)

target_link_libraries(app
    ${CMAKE_SOURCE_DIR}/lib/libglfw3.a
    opengl32
    gdi32
    user32
)