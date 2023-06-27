cmake_minimum_required(VERSION 3.16.0)

project(Fcpp VERSION 0.0.0 LANGUAGES CXX C Fortran)

# FIXME:
set(CMAKE_CXX_STANDARD 20)

#
# googletest
#
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

#include_directories(/usr/local/Cellar/eigen/3.4.0_1/include/eigen3/)

add_library(Fcpp INTERFACE)
target_include_directories(Fcpp INTERFACE include/)
target_compile_features(Fcpp INTERFACE cxx_std_20)

#add_executable(main main.cpp dot.f90)
#target_link_libraries(main Fcpp)

#add_executable(fill fill.f90 fill_increasing.cpp)
#target_compile_features(fill PUBLIC cxx_std_20)
#target_link_libraries(fill Fcpp)

enable_testing()

add_executable(
  cdesc_test
  test/cdesc_test.cc
  test/cdesc_alltwo.f90
)

target_link_libraries(
  cdesc_test
  Fcpp
  GTest::gtest_main
  gfortran
)

include(GoogleTest)
gtest_discover_tests(cdesc_test)