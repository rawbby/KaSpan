#!/bin/bash
# Run clang-tidy on the codebase.
# Usage: ./script/run_clang_tidy.sh [build_dir]

if ! command -v clang-tidy &> /dev/null; then
    echo "Error: clang-tidy is not installed."
    exit 1
fi

BUILD_DIR=${1:-cmake-build-debug}

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory $BUILD_DIR not found. Please build the project first."
    exit 1
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "compile_commands.json not found in $BUILD_DIR. Please run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON."
    exit 1
fi

# Run clang-tidy on scc, test, tool
# Auto-fix is enabled, and we explicitly set the header filter to ignore extern and system headers.
echo "Running clang-tidy with auto-fix..."
find scc test tool -type f \( -name "*.cpp" -o -name "*.c" \) -print0 | xargs -0 clang-tidy -p "$BUILD_DIR" --fix --fix-errors --header-filter='(scc|test|tool)/'
