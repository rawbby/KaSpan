#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=7
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_medium.out
#SBATCH -e hpc_graph_medium.err
#SBATCH -J hpc_graph_medium
#SBATCH --partition=cpuonly
#SBATCH --time=35:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh
source ~/workspace/KaSpan/experiment/horeka/run_parallel.sh
app_name=hpc_graph
app=~/workspace/KaSpan/cmake-build-release/bin/bench_hpc_graph
rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )
set +eu
for manifest in "${rwd[@]}"; do
manifest_name="$(basename "${manifest%.manifest}")"
run_async run_rwd "$app" "${app_name}_${manifest_name}_np152.json" "$manifest" "$app_name" 152 "$manifest_name" "--nodes=2 --ntasks=2 --cpus-per-task=76" --threads 76
run_async run_rwd "$app" "${app_name}_${manifest_name}_np304.json" "$manifest" "$app_name" 304 "$manifest_name" "--nodes=4 --ntasks=4 --cpus-per-task=76" --threads 76
wait_all
done
for manifest in "${rwd[@]}"; do
manifest_name="$(basename "${manifest%.manifest}")"
run_rwd "$app" "${app_name}_${manifest_name}_np532.json" "$manifest" "$app_name" 532 "$manifest_name" "--nodes=7 --ntasks=7 --cpus-per-task=76" --threads 76
done
