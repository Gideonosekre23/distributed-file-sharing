cmake_minimum_required(VERSION 3.10)
project(distributed-file-sharing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(distributed_file_sharing 
    main.cpp 
    node.cpp 
    utils.cpp
)

target_link_libraries(distributed_file_sharing ws2_32)
