cmake_minimum_required(VERSION 3.10.0)
project(MyProject)
aux_source_directory(src SRC_SUB)
add_executable(main ${SRC_SUB})
include_directories(include)
set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output)

# 查找并链接线程库
find_package(Threads REQUIRED)
target_link_libraries(main PRIVATE Threads::Threads)