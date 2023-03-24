#!/bin/bash -e

function cleanup {
  cd ..
}

cd build
# After we enter the directory we want to make sure we always exit it when the
# script ends.
trap cleanup EXIT

cmake --build . --target clean
