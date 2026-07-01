#!/usr/bin/env bash
set -eu

SRC="${SRC:-$(pwd)}"
OUT="${OUT:-${SRC}/out}"
WORK="${WORK:-${SRC}/work}"

mkdir -p "${OUT}" "${WORK}"

cmake -S "${SRC}" -B "${WORK}/build" \
  -DKNOTWORK_BUILD_TESTS=OFF \
  -DKNOTWORK_BUILD_FUZZER=ON \
  -DCMAKE_CXX_COMPILER="${CXX:-clang++}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS:-}"

cmake --build "${WORK}/build" --target scene_fuzzer
cp "${WORK}/build/scene_fuzzer" "${OUT}/scene_fuzzer"

if [ -d "${SRC}/corpus/valid" ]; then
  (cd "${SRC}/corpus/valid" && zip -q -r "${OUT}/scene_fuzzer_seed_corpus.zip" .)
fi
