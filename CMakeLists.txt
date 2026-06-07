cmake_minimum_required(VERSION 3.19)
project(WorldSync LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(.. ${CMAKE_BINARY_DIR}/sdk)

add_library(WorldSync SHARED
    src/main.cpp
    src/WorldSync.cpp
    src/Doormodule.cpp
    src/Cropmodule.cpp
    src/Pathmodule.cpp
    src/Vehiclemodule.cpp
)

target_link_libraries(WorldSync PUBLIC OMP-SDK)
target_include_directories(WorldSync PUBLIC include src)
target_compile_features(WorldSync PRIVATE cxx_std_17)

enable_testing()

add_executable(WorldSyncTests
    tests/WorldSyncTests.cpp
    src/WorldSync.cpp
    src/Cropmodule.cpp
    src/Pathmodule.cpp
    src/Vehiclemodule.cpp
)

target_link_libraries(WorldSyncTests PRIVATE OMP-SDK)
target_include_directories(WorldSyncTests PRIVATE include src)
target_compile_features(WorldSyncTests PRIVATE cxx_std_17)

add_test(NAME WorldSyncTests COMMAND WorldSyncTests)
