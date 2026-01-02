#!/bin/bash

if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi

# Find all .cpp, .hpp, .c, .h files in scc, test, tool, extern directories
# and run clang-format -i on them.

find scc test tool extern -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format -i
echo "Formatting complete."
