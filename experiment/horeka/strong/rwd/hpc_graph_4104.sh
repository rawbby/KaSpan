#!/bin/bash
#SBATCH --nodes=54
#SBATCH --ntasks=54
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_4104.out
#SBATCH -e hpc_graph_4104.err
#SBATCH -J hpc_graph_4104
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
output_file="${app_name}_${manifest_name}_np4104.json"
run_rwd "$app" "$output_file" "$manifest" "$app_name" 4104 "$manifest_name" "--nodes=54 --ntasks=54 --cpus-per-task=76" --threads 76
done
