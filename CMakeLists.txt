cmake_minimum_required(VERSION 3.16.0)

project(Fcpp VERSION 0.0.0 LANGUAGES CXX C Fortran)

option(FCPP_ENABLE_TESTS "Enable tests." Off)
option(FCPP_ENABLE_EXAMPLES "Build examples." Off)

# FIXME:
set(CMAKE_CXX_STANDARD 20)

add_library(Fcpp INTERFACE)
target_include_directories(Fcpp INTERFACE include/)
target_compile_features(Fcpp INTERFACE cxx_std_20)

if(FCPP_ENABLE_TESTS)

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()

#if(FCPP_ENABLE_EXAMPLES)
# add_subdirectory(examples)
#endif()