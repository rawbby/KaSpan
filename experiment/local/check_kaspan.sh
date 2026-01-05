#!/bin/bash
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

# Test sync version
echo "========================================================"
echo "Testing KaSpan (sync version)"
echo "========================================================"
run_benchmark "bench_kaspan"

# Test async version with NoopIndirectionScheme
echo ""
echo "========================================================"
echo "Testing KaSpan (async version - NoopIndirectionScheme)"
echo "========================================================"
run_benchmark "bench_kaspan" "--async"

# Test async version with GridIndirectionScheme
echo ""
echo "========================================================"
echo "Testing KaSpan (async version - GridIndirectionScheme)"
echo "========================================================"
run_benchmark "bench_kaspan" "--async --async_indirect"
