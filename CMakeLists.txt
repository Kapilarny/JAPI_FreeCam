cmake_minimum_required(VERSION 3.16)
project(JAPIPlugin C CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/japi)
add_subdirectory(external/glm)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

add_link_options(-static -static-libgcc -static-libstdc++)

add_library(JAPIPlugin SHARED src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "src" "external/japi/src/exports")
target_link_libraries(${PROJECT_NAME} PRIVATE d3dcompiler_47 d3d11 dbghelp glm)