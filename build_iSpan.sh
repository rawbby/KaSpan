#!/usr/bin/env bash
set -euo pipefail

dd="$(dirname "$0")"
cd "$dd"

echo "activating local spack environment"
spack env activate --create --dir .

echo "build iSpan graph converter"
mkdir -p "${dd}/solver/iSpan/graph_converter/build"
cd "${dd}/solver/iSpan/graph_converter/build"

make ..

echo "build iSpan distributed"
mkdir -p "${dd}/solver/iSpan/scr_mpi/build"
cd "${dd}/solver/iSpan/scr_mpi/build"

make ..

echo "copy iSpan binaries"
mkdir -p "${dd}/bin"
cd "${dd}/bin"

cp "${dd}/solver/iSpan/graph_converter/build/graph_converter" "${dd}/bin/iSpan_graph_converter"
cp "${dd}/solver/iSpan/scr_mpi/build/graph_converter" "${dd}/bin/iSpan"

echo "finished building iSpan"
