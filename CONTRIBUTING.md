# Contributing to KaSpan

Thank you for your interest in contributing to KaSpan! This document provides guidelines and information to help you get started.

## System Requirements

- **Operating System:** KaSpan is designed for **Linux only**. If you are on Windows, please use **WSL (Windows Subsystem for Linux)**.
- **Compiler:** A modern C++ compiler supporting the **C++23** standard is required.

## Development Tools

To maintain code quality, the following tools are required:

- **clang-format:** For code formatting.
- **clang-tidy:** For static analysis.
- **include-what-you-use (iwyu):** For managing includes.
- **valgrind:** For memory debugging and profiling.

We provide helper scripts in the `script/` directory to run these tools consistently.

### Using Development Scripts

The scripts are designed to be run from the project root on Linux/WSL:

```bash
./script/run_clang_format.sh          # Formats the codebase
./script/run_clang_tidy.sh            # Runs clang-tidy with auto-fix (requires build)
./script/run_iwyu.sh                  # Runs include-what-you-use
./script/run_valgrind.sh <executable> # Runs an executable with Valgrind
```

For scripts that require a build (like `clang-tidy`, `iwyu`, `cppcheck`), you can optionally pass the build directory as an argument (defaults to `cmake-build-debug`).

## Project Structure

- `scc/`: Contains the core library headers and implementation.
  - `include/kaspan/scc/`: Core SCC algorithms.
  - `include/kaspan/mpi_basic/`: MPI wrappers and communication primitives.
  - `include/kaspan/memory/`: Memory management utilities including `buffer`.
  - `include/kaspan/util/`: General utility functions and classes.
  - `include/kaspan/debug/`: Debugging and assertion tools.
- `extern/`: Snapshots of external projects and competitors used for benchmarking.
  - `ispan/`: Snapshot of the iSpan project.
  - `hpc_graph/`: Snapshot of the HPC Graph project.
  - `ispan_test/`: Integration tests for the iSpan project.
- `tool/`: Benchmarking tools and utility executables (e.g., `bench_kaspan`, `edgelist_converter`).
- `test/`: Unit and integration tests.
- `script/`: Helper scripts for development tools (formatting, linting, etc.).
- `experiment/`: Scripts and configurations for conducting experiments.

## Dependencies

- **MPI:** Required for distributed memory parallelism.
- **OpenMP:** Required for shared memory parallelism.
- **KaMPIng:** Used for MPI communication.
- **BriefKAsten:** Used for some communication patterns.
- **KaGen:** Used for graph generation.
- **stxxl:** Used for graph conversion utilities.

## Build and Testing

### Building the Project

The project uses CMake as its build system. **It is essential to export the compilation database** for static analysis tools to work correctly.

```bash
source /opt/intel/oneapi/setvars.sh # optional: if you are using intel oneapi this might be required
mkdir cmake-build-release
cd cmake-build-release
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build . -j 8
```

### Running Tests

KaSpan tests are unique in how they handle MPI. You should **not** run tests directly with `mpirun` or `mpiexec`. Instead, run the test executable directly:

```bash
source /opt/intel/oneapi/setvars.sh # optional: if you are using intel oneapi this might be required
mkdir cmake-build-debug
cd cmake-build-debug
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build . -j 8
ctest # optional: run all tests
./bin/test_scc_fuzzy
```

The test will automatically spawn `mpirun` subprocesses with different numbers of ranks (e.g., 1, 3, 8) to verify the logic across various configurations.

#### Fast Forward Testing

If you want to run a test instance explicitly (e.g., within an existing MPI environment or for debugging a specific rank configuration), you can pass the `--mpi-sub-process` flag. This flag prevents the test from spawning its own `mpirun` subprocesses.

```bash
mpirun -n 3 cmake-build-debug/bin/test_allgather_sub_graph --mpi-sub-process
```

## Coding Guidelines

### MPI Usage

Always use the **`kaspan::mpi_basic`** wrapper instead of calling MPI functions directly. `kaspan::mpi_basic` provides a safer and more consistent interface for the project's needs.

### Memory Management

To optimize performance and reduce allocations during execution, use **`kaspan::buffer`** (found in `scc/include/kaspan/memory/buffer.hpp`) whenever memory can be preallocated.

### Benchmarking Snapshots

The snapshots in `extern/ispan` and `extern/hpc_graph` are included as competitors for benchmarking purposes. These are fixed snapshots and **should not be changed** unless absolutely necessary for compatibility or critical bug fixes. Note that formatting scripts will still be applied to these directories to maintain a consistent style across the entire repository.

### Includes

To improve readability and facilitate manual dependency tracking, all `#include` directives should be followed by a comment listing the symbols used from that header.

```cpp
#include <vector> // std::vector
#include <kaspan/mpi_basic/world.hpp> // world_rank, world_size
```

## Development Tips

- Follow the existing code style and naming conventions.
- Ensure all new features are accompanied by relevant tests.
- Verify that your changes do not break existing benchmarks.
