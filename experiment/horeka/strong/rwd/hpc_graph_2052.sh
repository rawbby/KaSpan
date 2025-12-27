#!/bin/bash
#SBATCH --nodes=27
#SBATCH --ntasks=27
#SBATCH --cpus-per-task=76
#SBATCH --ntasks-per-socket=1
#SBATCH --ntasks-per-node=1
#SBATCH -o hpc_graph_2052.out
#SBATCH -e hpc_graph_2052.err
#SBATCH -J hpc_graph_2052
#SBATCH --partition=cpuonly
#SBATCH --time=25:00
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
  output_file="${app_name}_${manifest_name}_np2052.json"
  run_rwd "$app" "$output_file" "$manifest" "$app_name" 2052 "$manifest_name" "--nodes=27 --ntasks=27 --cpus-per-task=76" --threads 76
done