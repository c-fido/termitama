cmake_minimum_required(VERSION 3.14)
project(VirtualPet VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── FTXUI (auto-fetched at configure time) ────────────────────────────────────
include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG        v5.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ftxui)
# ─────────────────────────────────────────────────────────────────────────────

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
    src/TuiApp.cpp
)

target_include_directories(virtual_pet PRIVATE src)

if(MSVC)
    target_compile_options(virtual_pet PRIVATE /W4)
else()
    target_compile_options(virtual_pet PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(virtual_pet PRIVATE
    Threads::Threads
    ftxui::screen
    ftxui::dom
    ftxui::component
)
