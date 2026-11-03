#!/bin/sh
# Configure (first run) and build on Linux or macOS. Extra arguments are passed to ninja.
set -e
cd "$(dirname "$0")"

for tool in cmake ninja python3 clang; do
  command -v "$tool" >/dev/null 2>&1 || { echo "$tool not found on PATH. See README.md for requirements." >&2; exit 1; }
done

# The shared recompiler and runtime are a submodule.
[ -f gcn-recomp/CMakeLists.txt ] || git submodule update --init

if [ ! -f build/build.ninja ]; then
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
fi
ninja -C build "$@"
