project(libqtrabbitmq)

set(CMAKE_AUTOMOC ON)

include_directories(../include)

set(QMQ_SOURCES_CPP
  authentication.cpp
  channel.cpp
  client.cpp
  connection_handler.cpp
  consumer.cpp
  decimal.cpp
  frame.cpp
  message.cpp
  qtrabbitmq.cpp
  spec_constants.cpp
)

set(QMQ_HEADERS_MOC
)

set(QMQ_HEADERS
  ../include/qtrabbitmq/qtrabbitmq.h
  ../include/qtrabbitmq/abstract_frame_handler.h
  ../include/qtrabbitmq/authentication.h
  ../include/qtrabbitmq/client.h
  ../include/qtrabbitmq/channel.h
  ../include/qtrabbitmq/consumer.h
  ../include/qtrabbitmq/decimal.h
  ../include/qtrabbitmq/frame.h
  ../include/qtrabbitmq/message.h
  connection_handler.h
  spec_constants.h
)

set(QMQ_SOURCES
  ${QMQ_SOURCES_CPP}
  ${QMQ_HEADERS}
)

add_library(qtrabbitmq SHARED)

target_sources(qtrabbitmq PRIVATE ${QMQ_SOURCES})

target_include_directories(qtrabbitmq
  PUBLIC ${PUBLIC_INCLUDE_DIRS}
  PRIVATE ${PRIVATE_INCLUDE_DIRS}
)

target_compile_definitions(qtrabbitmq PRIVATE -DHAVE_CONFIG_H)

set(QMQ_LIBRARIES
  Qt6::Core
  Qt6::Network
)
target_link_libraries(qtrabbitmq ${QMQ_LIBRARIES})

set_target_properties(qtrabbitmq PROPERTIES
  VERSION ${QMQ_VERSION}
  SOVERSION ${QMQ_SOVERSION}
)

install(FILES
  ../include/qtrabbitmq.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

