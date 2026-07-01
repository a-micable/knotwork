# Knotwork

Knotwork is a C++20 scene and asset toolkit for small tools that need to load,
validate, inspect, transform, package, and fuzz structured 3D scene data. The
core format is `.kwscene`, a compact binary scene graph made from string tables,
materials, nodes, transforms, parent links, instance links, and typed node
metadata. Around that format, the project includes mesh utilities, package
archives, editor-style command replay, scene diffs, validation reports, and a
ClusterFuzzLite-ready fuzz target.

The project is intentionally built as a real library instead of a parser-only
toy. Most entry points run several stages: decode a structured container,
validate references, rebuild higher-level scene or project state, perform a
graph operation, then report or export derived data. That shape makes the code
useful as a local asset utility while also giving fuzzing more interesting
stateful paths than single-field length checks.

## What Knotwork Does

- Reads and writes `.kwscene` binary scene files.
- Validates parent graphs, instance graphs, string references, material
  references, and table boundaries.
- Flattens local transforms into world-space node transforms.
- Builds primitive meshes and computes mesh statistics.
- Loads and saves a compact binary mesh format.
- Loads and saves Wavefront OBJ for simple mesh exchange.
- Builds mesh BVHs and raycasts against meshes or scene assets.
- Manages project state with a scene, an asset library, and project files.
- Packages scenes, meshes, and text entries into a binary archive.
- Imports scene, package, OBJ, mesh, and text payloads through a shared import
  pipeline.
- Applies scene patches and editor commands with undo/redo.
- Computes scene diffs, scene audits, consistency checks, and validation
  reports.
- Renders tables, CSV, YAML-like summaries, schema docs, debug dumps, and
  project reports.
- Provides command-line tools for inspection, conversion, mesh generation,
  schema output, script parsing, hashing, and diagnostics.
- Ships a fuzz harness that drives scene loading, mesh loading, package
  loading, project reconstruction, editor replay, and import fallback paths.

## Repository Layout

```text
include/knotwork/        Public library headers
src/                     Library implementation and CLI entry point
tools/                   Focused command-line tools
tests/                   Self-contained regression test runner
fuzz/                    libFuzzer entry point
corpus/valid/            Valid seed scenes
.clusterfuzzlite/        ClusterFuzzLite build entry point
```

The library is intentionally header-separated from implementation files. Tools
link against the same `knotwork` library target that the tests and fuzzer use,
so build failures normally surface quickly across all executables.

## Build

Knotwork uses CMake and requires a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

A convenience script performs the same local verification:

```sh
./build.sh
```

Expected primary targets:

```text
knotwork              Static library
knotwork-info         Inspect a .kwscene file
knotwork-seed         Generate sample scenes
knotwork-doctor       Run project/scene diagnostics
knotwork-convert      Convert scene/package payloads
knotwork-mesh         Generate and inspect mesh assets
knotwork-script       Parse text scene scripts
knotwork-schema       Print schema documentation
knotwork-debug        Dump project or scene internals
knotwork-hash         Compute checksums
knotwork-tests        Regression test executable
scene_fuzzer          Optional fuzz target
```

## Quick Start

Generate a sample scene, inspect it, and run diagnostics:

```sh
./build/knotwork-seed /tmp/basic.kwscene basic
./build/knotwork-info /tmp/basic.kwscene
./build/knotwork-doctor /tmp/basic.kwscene
```

Generate an instanced scene:

```sh
./build/knotwork-seed /tmp/instanced.kwscene instanced
./build/knotwork-info /tmp/instanced.kwscene
```

Generate a mesh:

```sh
./build/knotwork-mesh box /tmp/box.kwmesh
./build/knotwork-mesh stats /tmp/box.kwmesh
```

Render schema documentation:

```sh
./build/knotwork-schema markdown
```

## Scene Model

A `Scene` contains three main tables:

- `strings`: shared string table used by nodes and materials.
- `materials`: material records with base color, roughness, and metallic values.
- `nodes`: scene graph records with stable IDs, node kind, parent index,
  optional material, optional instance target, local transform, and light/camera
  fields.

Node kinds:

```text
Empty
Mesh
Light
Camera
Instance
```

Validation checks that all referenced table entries exist, parent links form a
valid directed forest, and instance links do not create cycles. Flattening walks
the validated graph and computes a world matrix for every node. Instance nodes
combine their placement with the referenced target transform, which makes the
flattening pass a useful multi-record consistency check.

## Asset And Mesh Layer

Knotwork includes a small mesh asset layer that supports:

- Plane and box primitive generation.
- Vertex and triangle statistics.
- Bounds and surface-area calculation.
- Binary mesh save/load.
- OBJ save/load.
- BVH construction.
- Direct and accelerated raycast queries.
- Asset-library indexing by handle, name, source, and tags.

This layer lets tools build real project state instead of only manipulating raw
scene records. For example, a project can contain a `.kwscene`, a mesh library,
and package entries, then use inspection or raycast helpers that cross those
tables.

## Project And Package Layer

A `Project` combines scene data, asset data, project metadata, and file entries.
Packages serialize a group of named entries with kind tags and byte payloads.
Current package entry types include scenes, mesh binaries, OBJ text, generic
text, and binary blobs.

The import pipeline accepts structured requests and attempts to rebuild project
state from the payload:

- `.kwscene` bytes become scene data.
- package bytes become project files, scenes, and imported assets.
- mesh binary bytes become mesh assets.
- OBJ text becomes mesh assets.
- plain text becomes project file entries.

The importer reports counts and diagnostics instead of silently ignoring bad
payloads. That makes it useful for command-line conversion and fuzzing.

## Editor Replay And Patches

Knotwork has editor-oriented state helpers:

- Selection sets.
- Scene patches.
- Transform commands.
- Reparent commands.
- Undo/redo command history.
- Bookmarks.
- Status messages.
- Project inspection.

`EditorState::set_scene` clears selection and command history, which prevents
old commands from replaying against a replacement scene. Tests cover this
lifecycle behavior directly. The fuzzer also exercises it by applying commands,
replacing scene state, then attempting replay and inspection.

## Validation And Reports

The repository includes several reporting layers:

- `validate_scene` for strict scene graph validation.
- `validate_scene_detailed` for structured validation reports.
- `check_project_consistency` for scene/project/asset consistency.
- `audit_scene` for higher-level warnings.
- `inspect_scene` and `inspect_project` for summarized state.
- `diff_scenes` for comparing two scene graphs.
- `project_markdown_report` for human-readable project summaries.
- `schema_markdown` and `schema_text` for documented data layout.

These modules are used by the command-line tools and tests, so format changes
are easier to catch.

## Fuzzing

Build the local fuzz target:

```sh
cmake -S . -B fuzz-build -DKNOTWORK_BUILD_FUZZER=ON
cmake --build fuzz-build --target scene_fuzzer
./fuzz-build/scene_fuzzer corpus/valid
```

Build through the ClusterFuzzLite entry point:

```sh
OUT=/tmp/knotwork-out WORK=/tmp/knotwork-work SRC="$PWD" ./.clusterfuzzlite/build.sh
/tmp/knotwork-out/scene_fuzzer corpus/valid
```

The fuzzer intentionally drives several in-band paths from the same input:

- scene bytes: load -> validate -> flatten -> bounds
- mesh bytes: load -> BVH build -> raycast
- package bytes: load -> project reconstruction -> inspection
- editor replay: project setup -> selection -> command -> undo/redo -> patch
  -> scene replacement -> undo/redo -> inspection
- import pipeline: scene import -> package fallback import

That makes the harness stateful enough to exercise interactions between tables,
records, and replay phases while still keeping ownership explicit and safe.

## Tests

The regression suite is a single C++ executable for portability. It covers:

- scene binary round trips
- graph validation
- transform flattening
- instance flattening
- mesh primitive generation
- OBJ round trips
- asset-library search
- scene raycasting
- patch and diff behavior
- package round trips
- project reports
- camera visibility
- material previews
- layout helpers
- script parsing
- audit and CSV output
- keyframe merging
- pathfinding
- scene merge and unit scaling
- text table rendering
- validation and consistency reports
- string and checksum utilities
- schema and version helpers
- editor scene replacement clearing replay history

Run:

```sh
ctest --test-dir build --output-on-failure
```

## Development Principles

Knotwork should grow as a real codebase:

- Prefer structured parsers and explicit validation.
- Prefer value ownership, handles, and indices over raw pointer lifetime tricks.
- Keep fuzz harnesses close to production code paths.
- Keep command-line tools linked to the same library as tests and fuzzers.
- Add tests for lifecycle behavior when state replacement or replay is involved.
- Do not pad the repository with generated noise.
- Do not fake commit history.

## Current Status

Knotwork is a compact but functional C++20 project. It can build scenes, parse
and validate them, manipulate project state, package assets, run CLI tools, and
exercise multiple structured fuzzing paths. The next useful growth areas are
more seed corpus files, more package fixtures, richer material/animation
formats, and additional importer/exporter coverage.
