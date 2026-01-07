cmake_minimum_required(VERSION 3.16)
project(entt_cpp_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(entt_cpp_sim
    src/geo.cpp
    src/logging.cpp
    src/main.cpp
    src/route.cpp
    src/spatial_hash.cpp
    src/ent_simulation.cpp
)

target_include_directories(entt_cpp_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
