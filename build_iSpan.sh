#!/usr/bin/env bash
set -euo pipefail

dd="$(cd "$(dirname "$0")" && pwd)"
cd "$dd"

echo "activating local spack environment"
spack env activate --create --dir .

echo "build iSpan graph converter"
cd "${dd}/solver/iSpan/graph_converter"

g++ "txt_to_bin_fw_int.cpp" -o "${dd}/bin/iSpan_txt_to_bin_fw"
g++ "txt_to_bin_bw_int.cpp" -o "${dd}/bin/iSpan_txt_to_bin_bw"

echo "build iSpan (mpi)"
cd "${dd}/solver/iSpan/src_mpi"

make clean
make

echo "copy iSpan binaries"

mkdir -p "${dd}/bin"
mv "${dd}/solver/iSpan/src_mpi/scc_cpu" "${dd}/bin/iSpan"

echo "finished building iSpan"
