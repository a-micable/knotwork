# Knotwork

Knotwork is a compact C++20 scene-graph toolkit centered on a binary `.kwscene`
format. It stores named nodes, local transforms, parent edges, optional instance
references, materials, and a string table. The loader validates the graph and
the flattener computes world-space matrices for each node.

This repository is intended to become a substantial project through real
development history. It should not be padded with fake line count or synthetic
commits.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## CLI

```sh
./build/knotwork-seed /tmp/sample.kwscene instanced
./build/knotwork-info /tmp/sample.kwscene
```

## Fuzz Harness

The fuzzer loads arbitrary bytes as a scene, validates them, and runs the full
flattening path when validation succeeds.

```sh
cmake -S . -B fuzz-build -DKNOTWORK_BUILD_FUZZER=ON
cmake --build fuzz-build --target scene_fuzzer
./fuzz-build/scene_fuzzer corpus/valid
```
