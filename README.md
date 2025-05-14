# Distributed SCC Benchmarks

This repository contains benchmarks for a collection of distributed Strongly Connected Components (SCC) solvers.

The main purpose is to create a hub to collect different implementations and make them comparable.

## Usage

The scripts in this project expect a specific environment.  
This environment is created by each script if not already activated globally (which is recommended):

```shell
source scripts/env.sh
````

When the environment is loaded globally, all scripts and binaries are added to your `PATH`. Otherwise, add the
corresponding prefixes (`<repo_path>/scripts/<script>` and `<repo_path>/bin/<binary>`) to your calls.

### First-time Usage

On first use, ensure that all graph instances are downloaded and all solvers are built. To do so, run:

```shell
source scripts/env.sh
download_graphs.sh
build.sh
deactivate  # (optional) deactivate the environment
```

## Benchmarking

Currently available (WIP) benchmarks:

1. Strong scaling
2. Weak scaling
3. Selected small real-world instances (< 1 GB)
4. Selected medium real-world instances (> 1 GB)
5. Selected large real-world instances (> 100 GB)
6. Selected massive real-world instances (> 1 TB)

## Plotting

When a benchmark completes, it creates a report in the `results` folder.
You can create plots from a result directory using the plotting utility:

```shell
source scripts/env.sh
plot.py <result>  # where <result> is the folder name under results/<result>
deactivate        # (optional) deactivate the environment
```
