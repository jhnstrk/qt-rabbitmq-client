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
  # Ref: https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++ 
  add_compile_options(-Wall -Wextra -Wpedantic 
    -Wformat -Wformat=2 -Wconversion -Wimplicit-fallthrough 
    -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
    -D_GLIBCXX_ASSERTIONS
    # -fstrict-flex-arrays=3
    -fstack-clash-protection -fstack-protector-strong
    -Wl,-z,nodlopen -Wl,-z,noexecstack
    -Wl,-z,relro -Wl,-z,now
  )

  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
  )
endif()

add_subdirectory(src)

if (BUILD_TESTING)
  add_subdirectory(test)
endif()
