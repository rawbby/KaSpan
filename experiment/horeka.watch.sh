#!/bin/bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mapfile -t JOB_IDS < "${BASE_DIR}/jobs.txt"
IDS=()
for JOB_ID in "${JOB_IDS[@]}"; do
  [[ -n "${JOB_ID//[[:space:]]/}" ]] && IDS+=("${JOB_ID}")
done
if (( ${#IDS[@]} == 0 )); then
  echo "No job ids found in ${BASE_DIR}/jobs.txt" >&2
  exit 1
fi

JOB_ID_LIST="$(IFS=,; echo "${IDS[*]}")"
squeue --start -j "$JOB_ID_LIST"
