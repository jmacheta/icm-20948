include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(METADATA_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/icm-20948)

install(TARGETS icm-20948 EXPORT icm-20948-targets ARCHIVE FILE_SET HEADERS)
install(EXPORT icm-20948-targets NAMESPACE ecpp:: DESTINATION ${METADATA_DIR})

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/icm-20948-config_template.cmake ${CMAKE_CURRENT_BINARY_DIR}/icm-20948-config.cmake INSTALL_DESTINATION ${METADATA_DIR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(icm-20948-config-version.cmake COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/icm-20948-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/icm-20948-config-version.cmake
        DESTINATION ${METADATA_DIR}
)
