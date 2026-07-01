cmake_minimum_required(VERSION 3.20)
project(knotwork VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KNOTWORK_BUILD_TESTS "Build Knotwork tests" ON)
option(KNOTWORK_BUILD_FUZZER "Build libFuzzer harness" OFF)

add_library(knotwork
  src/animation.cpp
  src/asset_manifest.cpp
  src/asset_library.cpp
  src/bounds.cpp
  src/builder.cpp
  src/binary_reader.cpp
  src/binary_writer.cpp
  src/camera.cpp
  src/checksum.cpp
  src/consistency.cpp
  src/csv.cpp
  src/debug_dump.cpp
  src/diagnostics.cpp
  src/editor_state.cpp
  src/flatten.cpp
  src/format.cpp
  src/graph.cpp
  src/json.cpp
  src/import_pipeline.cpp
  src/keyframe_editor.cpp
  src/math.cpp
  src/material.cpp
  src/mesh.cpp
  src/mesh_io.cpp
  src/obj.cpp
  src/optimizer.cpp
  src/package.cpp
  src/patch.cpp
  src/pathfinding.cpp
  src/procedural.cpp
  src/project.cpp
  src/quality.cpp
  src/query.cpp
  src/raycast.cpp
  src/report.cpp
  src/schema.cpp
  src/selection.cpp
  src/scene_audit.cpp
  src/scene.cpp
  src/scene_diff.cpp
  src/scene_inspector.cpp
  src/scene_layout.cpp
  src/scene_merge.cpp
  src/scene_script.cpp
  src/spatial_index.cpp
  src/statistics.cpp
  src/string_util.cpp
  src/table.cpp
  src/text.cpp
  src/units.cpp
  src/validation_report.cpp
  src/version.cpp
  src/yaml.cpp
  src/validate.cpp
)

target_include_directories(knotwork PUBLIC include)
target_compile_options(knotwork PRIVATE -Wall -Wextra -Wpedantic)

add_executable(knotwork-info src/cli.cpp)
target_link_libraries(knotwork-info PRIVATE knotwork)

add_executable(knotwork-seed tools/make_seed.cpp)
target_link_libraries(knotwork-seed PRIVATE knotwork)

add_executable(knotwork-doctor tools/knotwork_doctor.cpp)
target_link_libraries(knotwork-doctor PRIVATE knotwork)

add_executable(knotwork-convert tools/knotwork_convert.cpp)
target_link_libraries(knotwork-convert PRIVATE knotwork)

add_executable(knotwork-mesh tools/knotwork_mesh.cpp)
target_link_libraries(knotwork-mesh PRIVATE knotwork)

add_executable(knotwork-script tools/knotwork_script.cpp)
target_link_libraries(knotwork-script PRIVATE knotwork)

add_executable(knotwork-schema tools/knotwork_schema.cpp)
target_link_libraries(knotwork-schema PRIVATE knotwork)

add_executable(knotwork-debug tools/knotwork_debug.cpp)
target_link_libraries(knotwork-debug PRIVATE knotwork)

add_executable(knotwork-hash tools/knotwork_hash.cpp)
target_link_libraries(knotwork-hash PRIVATE knotwork)

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
