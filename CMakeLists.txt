cmake_minimum_required(VERSION 3.20)
project(c_utils C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(c_utils INTERFACE)
target_include_directories(c_utils INTERFACE ${PROJECT_SOURCE_DIR})

if(MSVC)
  target_compile_options(c_utils INTERFACE /std:clatest)
else()
  target_compile_options(c_utils INTERFACE -std=c23)
endif()

set(TEST_SOURCES
  tests/test_darr.c
  tests/test_hashmap.c
  tests/test_strv.c
  tests/test_dstr.c
)

enable_testing()
set(CTEST_TEST_TIMEOUT 30)

foreach(test_src IN LISTS TEST_SOURCES)
  get_filename_component(test_name "${test_src}" NAME_WE)
  add_executable("${test_name}" "${test_src}")
  target_link_libraries("${test_name}" PRIVATE c_utils)

  if(MSVC)
    target_compile_options("${test_name}" PRIVATE /fsanitize=address)
    target_link_options("${test_name}" PRIVATE /fsanitize=address)
  else()
    target_compile_options("${test_name}" PRIVATE -fsanitize=address -fsanitize=undefined)
    target_link_options("${test_name}" PRIVATE -fsanitize=address -fsanitize=undefined)
  endif()

  add_test(NAME "${test_name}" COMMAND "${test_name}")
endforeach()

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
