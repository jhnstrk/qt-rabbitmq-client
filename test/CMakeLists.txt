find_package(Qt6 REQUIRED COMPONENTS Test)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
  "../src/include"
  "../src/libqtrabbitmq"
  "${QTRABBITMQ_ADD_INCLUDE_DIR}"
  )


set(CMAKE_AUTOMOC ON)

enable_testing(true)

set(test_items basic;frame_io;connect;pubsub;heartbeats)
foreach(item IN LISTS test_items)
  qt_add_executable(tst_${item} tst_${item}.cpp)
  add_test(NAME tst_${item} COMMAND tst_${item})
  target_link_libraries(tst_${item} PRIVATE Qt::Test qtrabbitmq)
endforeach()
