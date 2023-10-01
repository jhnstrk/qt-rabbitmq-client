find_package(Qt6 REQUIRED COMPONENTS Test)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("../src/include")
include_directories("../src/libqtrabbitmq")

set(CMAKE_AUTOMOC ON)

enable_testing(true)

qt_add_executable(tst_basic tst_basic.cpp)
add_test(NAME tst_basic COMMAND tst_basic)
target_link_libraries(tst_basic PRIVATE Qt::Test qtrabbitmq)

qt_add_executable(tst_frame_io tst_frame_io.cpp)
add_test(NAME tst_frame_io COMMAND tst_frame_io)
target_link_libraries(tst_frame_io PRIVATE Qt::Test qtrabbitmq)
