# - Find QtRabbitmq
# Find the QtRabbitmq includes and library
# This module defines
#  QTRABBITMQ_INCLUDE_DIR, where to find qtrabbitmq.h, etc.
#  QTRABBITMQ_LIBRARIES, the libraries needed to use QtRabbitmq.
#  QTRABBITMQ_FOUND, If false, do not try to use QtRabbitmq.

find_path(QTRABBITMQ_INCLUDE_DIR qtrabbitmq/qtrabbitmq.h
  PATHS
    /usr/local/include
    /usr/include
  DOC "Location of QtRabbitmq Headers"
)

set(QTRABBITMQ_NAMES ${QTRABBITMQ_NAMES} qtrabbitmq)
find_library(QTRABBITMQ_LIBRARY
  NAMES ${QTRABBITMQ_NAMES}
  PATHS /usr/lib /usr/local/lib
)

if (QTRABBITMQ_LIBRARY AND QTRABBITMQ_INCLUDE_DIR)
  set(QTRABBITMQ_LIBRARIES ${QTRABBITMQ_LIBRARY})
  set(QTRABBITMQ_FOUND "YES")
else (QTRABBITMQ_LIBRARY AND QTRABBITMQ_INCLUDE_DIR)
  set(QTRABBITMQ_FOUND "NO")
endif (QTRABBITMQ_LIBRARY AND QTRABBITMQ_INCLUDE_DIR)


if (QTRABBITMQ_FOUND)
   if (NOT QTRABBITMQ_FIND_QUIETLY)
      message(STATUS "Found QtRabbitmq: ${QTRABBITMQ_LIBRARIES}")
   endif (NOT QTRABBITMQ_FIND_QUIETLY)
else (QTRABBITMQ_FOUND)
   if (QTRABBITMQ_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find QtRabbitmq library")
   endif (QTRABBITMQ_FIND_REQUIRED)
endif (QTRABBITMQ_FOUND)

MARK_AS_ADVANCED(QTRABBITMQ_LIBRARY QTRABBITMQ_INCLUDE_DIR)
