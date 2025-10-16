#!/bin/bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ ! -f "${BASE_DIR}/jobs.txt" ]]; then
  echo "No jobs file: ${BASE_DIR}/jobs.txt" >&2
  exit 1
fi

while IFS= read -r JOB_ID || [[ -n "${JOB_ID}" ]]; do
  [[ -z "${JOB_ID//[[:space:]]/}" ]] && continue
  scancel "${JOB_ID}" && printf 'Cancelled %s\n' "${JOB_ID}" || printf 'Failed to cancel %s\n' "${JOB_ID}"
done < "${BASE_DIR}/jobs.txt"
