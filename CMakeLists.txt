cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

project(Gravitaugmented VERSION 0.1.0 LANGUAGES C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake)

set(AUGMENT_FFI OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/minhook)
add_library(minhook::minhook ALIAS minhook)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/augment)

add_executable(gravitas ${SOURCES})
set_target_properties(gravitas PROPERTIES OUTPUT_NAME "Gravitaugmented")
target_link_libraries(gravitas augment minhook)
