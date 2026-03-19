cmake_minimum_required(VERSION 3.11)
project(Game C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(raylib)

add_executable(game game.c)
target_link_libraries(game raylib)