add_library(icm-20948)
add_library(emc::icm-20948 ALIAS icm-20948)

target_compile_features(icm-20948 PUBLIC cxx_std_23)

target_sources(
  icm-20948
  PRIVATE icm-20948.cpp
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         include
         FILES
         include/icm-20948/icm-20948.hpp
         include/icm-20948/registers.hpp
  PRIVATE FILE_SET private_headers TYPE HEADERS BASE_DIRS include_private
)
