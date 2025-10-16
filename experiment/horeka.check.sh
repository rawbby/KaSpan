#!/bin/bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ANY=0

shopt -s nullglob
for ERROR in "${BASE_DIR}"/horeka.job.*.err; do
  if [[ -f "${ERROR}" ]] && [[ -s "${ERROR}" ]]; then
    ANY=1
    echo ""
    echo "${ERROR}"
    echo '```bash'
    cat "${ERROR}"
    echo '```'
  fi
done
shopt -u nullglob

exit $ANY
