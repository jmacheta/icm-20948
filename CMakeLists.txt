cmake_minimum_required(VERSION 3.25)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)
project(icm-20948 VERSION ${PROJECT_VERSION})

option(ICM20948_TESTS "Build test cases" ${PROJECT_IS_TOP_LEVEL})
option(ICM20948_INSTALL "Configure install target" ON)

include(cmake/requirements.cmake)

add_subdirectory(src)

if (ICM20948_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif ()

if (ICM20948_INSTALL)
  include(cmake/install.cmake)
endif ()
