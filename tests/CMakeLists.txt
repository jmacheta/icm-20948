add_executable(icm-20948-tests)
target_compile_features(icm-20948-tests PRIVATE cxx_std_23)

target_sources(icm-20948-tests PRIVATE icm-20948.test.cpp)

target_link_libraries(icm-20948-tests PRIVATE emc::icm-20948 GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(icm-20948-tests)
