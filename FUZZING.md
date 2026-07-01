# Fuzzing

Knotwork's fuzzer does more than parse a single table. It attempts multiple
structured entry points from the same byte stream:

- scene binary load, validation, hierarchy flattening, and bounds setup
- mesh binary load, BVH construction, and ray traversal
- package load, project reconstruction, and project inspection
- import-pipeline execution for scene and package payloads

The seed corpus in `corpus/valid` contains valid scene files that reach the
scene pipeline. Additional mesh and package seeds can be produced with the CLI
tools after building.

ClusterFuzzLite entry point:

```sh
OUT=/tmp/knotwork-out WORK=/tmp/knotwork-work SRC="$PWD" ./.clusterfuzzlite/build.sh
/tmp/knotwork-out/scene_fuzzer corpus/valid
```

Local CMake build:

```sh
cmake -S . -B fuzz-build -DKNOTWORK_BUILD_FUZZER=ON
cmake --build fuzz-build --target scene_fuzzer
./fuzz-build/scene_fuzzer corpus/valid
```

The editor replay path intentionally exercises project lifecycle transitions:
it builds or imports a project, applies command history, replaces the active
scene, then attempts undo/redo and inspection. The expected behavior is clean
failure or clean replay, never stale state access.
