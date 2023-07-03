add_executable(
  cdesc_test
  cdesc_test.cc
  cdesc_alltwo.f90
)

# FIXME: currently gcc/gfortran only
target_link_libraries(
  cdesc_test
  Fcpp
  GTest::gtest_main
  gfortran
)

include(GoogleTest)
gtest_discover_tests(cdesc_test)

add_executable(iota_test iota_test.f90 iota.cpp)
target_link_libraries(iota_test Fcpp)