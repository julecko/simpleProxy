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
(
  cd "$BUILD_DIR"
  cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../..
  make

  if [ "$BUILD_TYPE" == "Release" ]; then
    echo "Generating .deb package..."
    cpack -G DEB
  fi
)

if [ "$RUN_EXEC" = true ]; then
  echo "Running ./simpleproxy ($BUILD_TYPE build)"
  "./$BUILD_DIR/simpleproxy"
fi
