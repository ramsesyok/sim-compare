cmake_minimum_required(VERSION 3.16)

project(oop_cpp_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(oop_cpp_sim
    main.cpp
    simulation.cpp
    loggin.cpp
    geo.cpp
    route.cpp
    spatial_hash.cpp
    sim_object.cpp
    fixed_object.cpp
    movable_object.cpp
    commander_object.cpp
    scout_object.cpp
    messenger_object.cpp
    attacker_object.cpp
)

target_include_directories(oop_cpp_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
