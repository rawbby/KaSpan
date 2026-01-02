#!/bin/bash
# Run a specific test executable with valgrind.
# Usage: ./script/run_valgrind.sh <test_executable> [test_args...]

if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_executable> [test_args...]"
    echo "Example: $0 cmake-build-debug/bin/test_scc_fuzzy"
    echo ""
    echo "Note: This script uses --trace-children=yes by default to profile MPI subprocesses spawned by the tests."
    exit 1
fi

if ! command -v valgrind &> /dev/null; then
    echo "Error: valgrind is not installed."
    exit 1
fi

TEST_EXE=$1
shift

# Default valgrind args
VALGRIND_ARGS="--leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes"

# Run valgrind
valgrind $VALGRIND_ARGS "$TEST_EXE" "$@"
