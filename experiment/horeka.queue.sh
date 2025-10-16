#!/bin/bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

: > "${BASE_DIR}/jobs.txt"

shopt -s nullglob
for JOB in "${BASE_DIR}"/horeka.job.*.sh; do
  JOB_ID=$(sbatch --parsable "${JOB}") || { echo "sbatch failed for ${JOB}" >&2; continue; }
  echo "${JOB_ID}" >> "${BASE_DIR}/jobs.txt"
  printf 'Submitted %s -> %s\n' "${JOB}" "${JOB_ID}"
done
shopt -u nullglob

printf 'Wrote job ids to %s\n' "${BASE_DIR}/jobs.txt"
