cmake_minimum_required(VERSION 3.17)
project(zombie_flood)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find SDL2_image (Manual)
find_library(SDL2_IMAGE_LIB NAMES SDL2_image PATHS /opt/homebrew/lib REQUIRED)
find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h PATHS /opt/homebrew/include/SDL2 /opt/homebrew/include REQUIRED)

include_directories(${SDL2_IMAGE_INCLUDE_DIR} /opt/homebrew/include /opt/homebrew/include/SDL2)

# Add executable
add_executable(zombie_flood src/main.cpp src/Game.cpp src/Map.cpp src/Simulation.cpp)

# Link libraries
target_link_libraries(zombie_flood ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIB} OpenGL::GL)
