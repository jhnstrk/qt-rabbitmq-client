project(libqtrabbitmq)

include(GenerateExportHeader)

set(CMAKE_AUTOMOC ON)

include_directories(../include)
# For the export macros:
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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

add_library(qtrabbitmq)

generate_export_header(qtrabbitmq
  EXPORT_FILE_NAME "${QTRABBITMQ_ADD_INCLUDE_DIR}/qtrabbitmq_export.h"
)

target_sources(qtrabbitmq PRIVATE ${QMQ_SOURCES})

target_include_directories(qtrabbitmq
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include;${CMAKE_CURRENT_BINARY_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
   INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
#  PRIVATE ${PRIVATE_INCLUDE_DIRS}
)
# target_compile_definitions(qtrabbitmq PRIVATE -DHAVE_CONFIG_H)

set(QMQ_LIBRARIES
  Qt6::Core
  Qt6::Network
)
target_link_libraries(
  qtrabbitmq 
  ${QMQ_LIBRARIES}
)

set_target_properties(qtrabbitmq PROPERTIES
  VERSION ${QMQ_VERSION}
  SOVERSION ${QMQ_SOVERSION}
)

install(FILES
  ../include/qtrabbitmq/abstract_frame_handler.h
  ../include/qtrabbitmq/authentication.h
  ../include/qtrabbitmq/channel.h
  ../include/qtrabbitmq/client.h
  ../include/qtrabbitmq/consumer.h
  ../include/qtrabbitmq/decimal.h
  ../include/qtrabbitmq/exception.h
  ../include/qtrabbitmq/frame.h
  ../include/qtrabbitmq/message.h
  ../include/qtrabbitmq/qtrabbitmq.h
  "${QTRABBITMQ_ADD_INCLUDE_DIR}/qtrabbitmq_export.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtrabbitmq/"
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(QtRabbitmq_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/QtRabbitmq"
    CACHE STRING "Path to QtRabbitmq CMake files")

install(EXPORT qtrabbitmq_Targets
        DESTINATION "${QtRabbitmq_INSTALL_CMAKEDIR}"
        # NAMESPACE QtRabbitmq::
        FILE QtRabbitmqConfig.cmake
        COMPONENT QtRabbitmq_Development)

write_basic_package_version_file(
    QtRabbitmqConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    VERSION "${QMQ_VERSION}")

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtRabbitmqConfigVersion.cmake"
        DESTINATION "${QtRabbitmq_INSTALL_CMAKEDIR}"
        COMPONENT QtRabbitmq_Development)

install(TARGETS qtrabbitmq
        EXPORT qtrabbitmq_Targets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/qtrabbitmq)
