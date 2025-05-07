#!/usr/bin/env bash
set -euo pipefail

dd="$(dirname "$0")"
cd "$dd"

spack env activate --create .
python3 -m venv .venv

# Synchronize submodule URLs
# This updates .git/config to match .gitmodules
echo "Synchronizing submodule URLs..."
git submodule sync --recursive

# Initialize and update submodules to recorded commits
echo "Initializing submodules..."
git submodule update --init --recursive

# Completion message
echo "Bootstrap complete: all submodules are at the recorded commits."
