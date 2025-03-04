cmake_minimum_required(VERSION 3.10)

project(QtHelloWorld)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find Qt5 libraries
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Include the src directory
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add executable
add_executable(QtHelloWorld src/main.cpp)

# Link Qt libraries
target_link_libraries(QtHelloWorld Qt5::Widgets)
