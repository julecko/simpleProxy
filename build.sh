#!/bin/bash

set -e

BUILD_TYPE="Debug"

if [[ "$1" == "--release" ]]; then
  BUILD_TYPE="Release"
fi

BUILD_DIR="build/${BUILD_TYPE,,}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../..
make

echo "Running ./simpleProxy ($BUILD_TYPE build)"
./simpleProxy
