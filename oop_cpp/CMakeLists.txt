cmake_minimum_required(VERSION 3.16)

project(oop_cpp_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(oop_cpp_lib
    src/simulation.cpp
    src/logging.cpp
    src/geo.cpp
    src/route.cpp
    src/spatial_hash.cpp
    src/sim_object.cpp
    src/fixed_object.cpp
    src/movable_object.cpp
    src/commander_object.cpp
    src/scout_object.cpp
    src/messenger_object.cpp
    src/attacker_object.cpp
)

target_include_directories(oop_cpp_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include
)

add_executable(oop_cpp_sim src/main.cpp)
target_link_libraries(oop_cpp_sim PRIVATE oop_cpp_lib)
target_include_directories(oop_cpp_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include
)

enable_testing()

add_executable(oop_cpp_tests
    tests/test_sim.cpp
    tests/test_fixed_object.cpp
    tests/test_movable_object.cpp
    tests/test_commander_object.cpp
    tests/test_messenger_object.cpp
    tests/test_attacker_object.cpp
    tests/test_scout_object.cpp
    tests/catch_amalgamated.cpp
)
target_include_directories(oop_cpp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(oop_cpp_tests PRIVATE oop_cpp_lib)

add_test(NAME oop_cpp_tests COMMAND oop_cpp_tests)
