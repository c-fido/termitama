cmake_minimum_required(VERSION 3.14)
project(VirtualPet VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(virtual_pet
    src/main.cpp

    src/Pet.cpp
    src/Display.cpp

    src/Item.cpp
    src/Inventory.cpp
    src/Shop.cpp

    src/PetFSM.cpp

    src/AudioManager.cpp

    src/Genetics.cpp

    src/Minigame.cpp
    src/GameManager.cpp
)

target_include_directories(virtual_pet PRIVATE src)

if(MSVC)
    target_compile_options(virtual_pet PRIVATE /W4)
else()
    target_compile_options(virtual_pet PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(virtual_pet PRIVATE Threads::Threads)
