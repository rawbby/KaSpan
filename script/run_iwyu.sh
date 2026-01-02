#!/bin/bash
# Run include-what-you-use on the codebase.
# Usage: ./script/run_iwyu.sh [build_dir]

BUILD_DIR=${1:-cmake-build-debug}

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "compile_commands.json not found in $BUILD_DIR."
    exit 1
fi

# iwyu_tool is usually the preferred way to run iwyu on a project with compile_commands.json
if command -v iwyu_tool.py &> /dev/null; then
    iwyu_tool.py -p "$BUILD_DIR" scc test tool extern
elif command -v iwyu_tool &> /dev/null; then
    iwyu_tool -p "$BUILD_DIR" scc test tool extern
else
    echo "iwyu_tool.py not found. Please install include-what-you-use."
    exit 1
fi
