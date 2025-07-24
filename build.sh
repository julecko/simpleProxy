#!/bin/bash

set -e

BUILD_TYPE="Debug"
RUN_EXEC=false

for arg in "$@"; do
  case "$arg" in
    --release) BUILD_TYPE="Release" ;;
    -r) RUN_EXEC=true ;;
  esac
done

BUILD_DIR="build/${BUILD_TYPE,,}"

mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"

if [ "$BUILD_TYPE" == "Release" ]; then
  echo "Generating .deb package..."
  cmake --build "$BUILD_DIR" --target package
fi

if [ "$RUN_EXEC" = true ]; then
  echo "Running ./simpleproxy ($BUILD_TYPE build)"
  "./$BUILD_DIR/simpleproxy"
fi
