find_package(Qt6 REQUIRED COMPONENTS Test)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

enable_testing(true)

qt_add_executable(tst_basic tst_basic.cpp)

add_test(NAME tst_basic COMMAND tst_basic)

target_link_libraries(tst_basic PRIVATE Qt::Test qtrabbitmq)
