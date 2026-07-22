cmake_minimum_required(VERSION 3.20)
project(knotwork VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)

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

target_include_directories(knotwork PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_options(knotwork PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(knotwork PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  EXPORT_NAME knotwork
)
add_library(knotwork::knotwork ALIAS knotwork)

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

install(TARGETS knotwork
  EXPORT knotworkTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS
  knotwork-info
  knotwork-seed
  knotwork-doctor
  knotwork-convert
  knotwork-mesh
  knotwork-script
  knotwork-schema
  knotwork-debug
  knotwork-hash
  EXPORT knotworkTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(KNOTWORK_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/knotwork")
include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KnotworkConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KnotworkConfig.cmake"
  INSTALL_DESTINATION "${KNOTWORK_CONFIG_INSTALL_DIR}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/KnotworkConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KnotworkConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KnotworkConfigVersion.cmake"
  DESTINATION "${KNOTWORK_CONFIG_INSTALL_DIR}"
)

install(EXPORT knotworkTargets
  NAMESPACE knotwork::
  FILE knotworkTargets.cmake
  DESTINATION "${KNOTWORK_CONFIG_INSTALL_DIR}"
)

export(EXPORT knotworkTargets FILE "${CMAKE_CURRENT_BINARY_DIR}/knotworkTargets.cmake")

set(CPACK_PACKAGE_NAME "knotwork")
set(CPACK_PACKAGE_VENDOR "Knotwork Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Knotwork scene and asset toolkit")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)

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
