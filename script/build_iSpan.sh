#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

build_graph_converter() {
   >&2 echo "[iSpan] building graph converter ..."
  cd "${PROJECT_DIR}/solver/iSpan/graph_converter"
  g++ "txt_to_bin_fw_int.cpp" -o "${BINARY_DIR}/iSpan_txt_to_bin_fw"
  g++ "txt_to_bin_bw_int.cpp" -o "${BINARY_DIR}/iSpan_txt_to_bin_bw"
   >&2 echo "[iSpan] built graph converter!"
}

build_iSpan() {
   >&2 echo "[iSpan] building (mpi) ..."
  cd "${PROJECT_DIR}/solver/iSpan/src_mpi"
  make clean
  make
  mv "${PROJECT_DIR}/solver/iSpan/src_mpi/scc_cpu" "${BINARY_DIR}/iSpan"
   >&2 echo "[iSpan] built (mpi)!"
}

build_graph_converter
build_iSpan
deactivate  # deactivate env.sh
