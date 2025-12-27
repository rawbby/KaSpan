#!/bin/bash
#SBATCH --nodes=14
#SBATCH --ntasks=14
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_1064.out
#SBATCH -e hpc_graph_1064.err
#SBATCH -J hpc_graph_1064
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh
app_name=hpc_graph
app=~/workspace/KaSpan/cmake-build-release/bin/bench_hpc_graph
rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )
set +eu
for manifest in "${rwd[@]}"; do
manifest_name="$(basename "${manifest%.manifest}")"
output_file="${app_name}_${manifest_name}_np1064.json"
run_rwd "$app" "$output_file" "$manifest" "$app_name" 1064 "$manifest_name" "--nodes=14 --ntasks=14 --cpus-per-task=76" --threads 76
done
