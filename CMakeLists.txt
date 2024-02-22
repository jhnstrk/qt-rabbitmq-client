cmake_minimum_required(VERSION 3.16...3.22)

set(QMQ_VERSION "0.1.0")
set(QMQ_SOVERSION ${QMQ_VERSION})

project(qtrabbitmq
  VERSION ${QMQ_VERSION}
  DESCRIPTION "Qt RabbitMQ AMQP client library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTING "Build test suite" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

include(GNUInstallDirs)

# Compile options
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
  )
endif()

add_subdirectory(src)

if (BUILD_TESTING)
  add_subdirectory(test)
endif()
