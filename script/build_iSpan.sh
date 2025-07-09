#!/usr/bin/env bash
source "$(dirname "$0")/env.sh"

build_iSpan() {
   >&2 echo "[iSpan] building (mpi) ..."

  mkdir -p "${PROJECT_DIR}/solver/iSpan/cmake-build-release"
  cd "${PROJECT_DIR}/solver/iSpan/cmake-build-release"

  cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_MPI=ON
  cmake --build .

  mv "${PROJECT_DIR}/solver/iSpan/cmake-build-release/bin/iSpan_mpi" "${BINARY_DIR}/iSpan"
  mv "${PROJECT_DIR}/solver/iSpan/cmake-build-release/bin/txt_to_bin_fw" "${BINARY_DIR}/iSpan_txt_to_bin_fw"
  mv "${PROJECT_DIR}/solver/iSpan/cmake-build-release/bin/txt_to_bin_bw" "${BINARY_DIR}/iSpan_txt_to_bin_bw"
   >&2 echo "[iSpan] built (mpi)!"
}

build_iSpan
deactivate  # deactivate env.sh
