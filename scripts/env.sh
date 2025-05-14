#!/usr/bin/env bash

# USAGE:
#   source <scripts_path>/env.sh
# TODO:
#   - create command line option to (not) use spack
#   - create command line option to force re-initialisation

# 1) Early exit if already sourced
if [[ -n "${ENV_COUNT:-}" ]]; then
  ENV_COUNT=$(( ENV_COUNT + 1 ))
  export ENV_COUNT
  >&2 echo "[env.sh] debug: ENV_COUNT=${ENV_COUNT}"
  return 0
fi

>&2 echo "[env.sh] activating ..."

# 2) Initialise environment counter
ENV_COUNT=1
export ENV_COUNT
>&2 echo "[env.sh] debug: ENV_COUNT=${ENV_COUNT}"

# 3) Strict error handling
set -euo pipefail
trap '>&2 echo "[env.sh] Error on or near line $LINENO. Exiting."; exit 1' ERR

# 4) Locate script & project roots
export SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export PROJECT_DIR="$( cd "${SCRIPTS_DIR}/.." && pwd )"
export BINARY_DIR="${PROJECT_DIR}/bin"
mkdir -p "${BINARY_DIR}"
cd "${PROJECT_DIR}"

# 5) Sync and Update git submodules
>&2 echo "[env.sh] note: syncing git submodules…"
git submodule sync --recursive
>&2 echo "[env.sh] note: updating git submodules…"
git submodule update --init --recursive

# 6) Add our scripts dir to PATH
export PATH="${SCRIPTS_DIR}:${PATH}"

# 7) Spack environment
>&2 echo "[env.sh] activating spack env in \"${PROJECT_DIR}\" ..."
spack env activate --create --dir .
>&2 echo "[env.sh] spack env activated!"

# 8) Python environment
if [[ ! -d ".venv" ]]; then
  >&2 echo "[env.sh] creating python venv at \"${PROJECT_DIR}/.venv\" ..."
  python3 -m venv "${PROJECT_DIR}/.venv"
fi
>&2 echo "[env.sh] activating python venv in \"${PROJECT_DIR}/.venv\" ..."
source ".venv/bin/activate"
>&2 echo "[env.sh] activated python venv!"

# If venv defines a `deactivate` function, rename it so we can chain
if declare -F deactivate >/dev/null; then
  eval "$(declare -f deactivate | sed '1s/deactivate/venv_deactivate/')"
  >&2 echo "[env.sh] note: renamed python venv's deactivate to venv_deactivate"
fi

# 9) Unified teardown
deactivate() {
  ENV_COUNT=$(( ENV_COUNT - 1 ))
  export ENV_COUNT
  >&2 echo "[env.sh] debug: ENV_COUNT=${ENV_COUNT}"

  if (( ENV_COUNT > 0 )); then
    return 0
  fi

  >&2 echo "[env.sh] deactivating ..."

  # c) Deactivate python venv
  if declare -F venv_deactivate >/dev/null; then
    >&2 echo "[env.sh] deactivating python venv ..."
    venv_deactivate
    unset -f venv_deactivate
    >&2 echo "[env.sh] deactivated python venv!"
  fi

  # d) Deactivate spack
  >&2 echo "[env.sh] deactivating spack env ..."
  spack env deactivate
  >&2 echo "[env.sh] deactivated spack env!"

  # e) Remove scripts dir from PATH
  PATH="${PATH#${SCRIPTS_DIR}:}"
  export PATH

  # f) Cleanup our exports
  unset ENV_COUNT
  unset SCRIPTS_DIR
  unset PROJECT_DIR

  # g) Remove this function
  unset -f deactivate

  # h) Remove autocomplete for deactivate
  complete -r deactivate

  # i) restore shell options
  trap - ERR
  set +euo pipefail

  >&2 echo "[env.sh] deactivated!"
}
export -f deactivate

# 10) Bash completion for deactivate command
_deactivate_complete() {
  # no subcommands yet, but register deactivate itself
  COMPREPLY=( $(compgen -W "" -- "${COMP_WORDS[1]:-}") )
}
complete -F _deactivate_complete deactivate

>&2 echo "[env.sh] activated!"
