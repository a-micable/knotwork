cmake_minimum_required(VERSION 3.20)
project(knotwork VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KNOTWORK_BUILD_TESTS "Build Knotwork tests" ON)
option(KNOTWORK_BUILD_FUZZER "Build libFuzzer harness" OFF)

add_library(knotwork
  src/binary_reader.cpp
  src/binary_writer.cpp
  src/flatten.cpp
  src/format.cpp
  src/math.cpp
  src/scene.cpp
  src/validate.cpp
)

target_include_directories(knotwork PUBLIC include)
target_compile_options(knotwork PRIVATE -Wall -Wextra -Wpedantic)

add_executable(knotwork-info src/cli.cpp)
target_link_libraries(knotwork-info PRIVATE knotwork)

add_executable(knotwork-seed tools/make_seed.cpp)
target_link_libraries(knotwork-seed PRIVATE knotwork)

if(KNOTWORK_BUILD_TESTS)
  enable_testing()
  add_executable(knotwork-tests tests/test_main.cpp)
  target_link_libraries(knotwork-tests PRIVATE knotwork)
  add_test(NAME knotwork-tests COMMAND knotwork-tests)
endif()

if(KNOTWORK_BUILD_FUZZER)
  add_executable(scene_fuzzer fuzz/scene_fuzzer.cc)
  target_link_libraries(scene_fuzzer PRIVATE knotwork)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(scene_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(scene_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
  else()
    target_compile_definitions(scene_fuzzer PRIVATE KNOTWORK_STANDALONE_FUZZER=1)
    target_compile_options(scene_fuzzer PRIVATE -fsanitize=address,undefined)
    target_link_options(scene_fuzzer PRIVATE -fsanitize=address,undefined)
  endif()
endif()
