
project(rabbit-tutorial-examples)

cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})

message(STATUS ${CMAKE_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core)

# Ensure CMAKE_MODULES_PATH include the path to FindQtRabbitmq.cmake
find_package(QtRabbitmq REQUIRED)

include_directories(${QTRABBITMQ_INCLUDE_DIR})

add_executable(01_hello_world 01_hello_world.cpp)
target_link_libraries(01_hello_world
  PRIVATE
    qtrabbitmq  
    Qt6::Core
)
