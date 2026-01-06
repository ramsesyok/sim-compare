cmake_minimum_required(VERSION 3.16)
project(soa_cpp_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(soa_cpp_sim
    src/geo.cpp
    src/logging.cpp
    src/main.cpp
    src/route.cpp
    src/spatial_hash.cpp
    src/simulation.cpp
)

target_include_directories(soa_cpp_sim PRIVATE
    3rdparty/include
    include
)
