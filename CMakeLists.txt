cmake_minimum_required(VERSION 3.14)
project(VirtualPet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source files
add_executable(virtual_pet
    src/main.cpp
    src/Pet.cpp
    src/Display.cpp
    src/Minigame.cpp
    src/GameManager.cpp
)

target_include_directories(virtual_pet PRIVATE src)

# Enable warnings
if(MSVC)
    target_compile_options(virtual_pet PRIVATE /W4)
else()
    target_compile_options(virtual_pet PRIVATE -Wall -Wextra -Wpedantic)
endif()
