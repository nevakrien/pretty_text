cmake_minimum_required(VERSION 3.5)
project(QtHelloWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
find_package(OpenGL REQUIRED)  # <-- add this

add_executable(QtHelloWorld
    src/main.cpp
)

# Link to Qt libraries + system OpenGL
target_link_libraries(QtHelloWorld
    Qt5::Widgets
    Qt5::OpenGL
    GL
)
