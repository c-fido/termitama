cmake_minimum_required(VERSION 3.14)
project(virtual_pet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch FTXUI
include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Source files
set(SOURCES
  src/main.cpp
  src/AudioManager.cpp
  src/Display.cpp
  src/GameManager.cpp
  src/Genetics.cpp
  src/Inventory.cpp
  src/Item.cpp
  src/Minigame.cpp
  src/Pet.cpp
  src/PetFSM.cpp
  src/Shop.cpp
  src/TuiApp.cpp
)

# Create executable
add_executable(virtual_pet ${SOURCES})

# Link FTXUI
target_link_libraries(virtual_pet PUBLIC ftxui::screen ftxui::component)

# Include directories
target_include_directories(virtual_pet PRIVATE src)
