#!/bin/bash
#SBATCH --nodes=7
#SBATCH --ntasks=532
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_medium.out
#SBATCH -e kaspan_medium.err
#SBATCH -J kaspan_medium
#SBATCH --partition=cpuonly
#SBATCH --time=35:00
#SBATCH --export=ALL

set -euo pipefail

source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh
source ~/workspace/KaSpan/experiment/horeka/run_parallel.sh


app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan


rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )


set +eu

for manifest in "${rwd[@]}"; do
  manifest_name="$(basename "${manifest%.manifest}")"
  run_async run_rwd "$app" "${app_name}_${manifest_name}_np152.json" "$manifest" "$app_name" 152 "$manifest_name" "--nodes=2 --ntasks=152 --cpus-per-task=1 --hint=nomultithread"
  run_async run_rwd "$app" "${app_name}_${manifest_name}_np304.json" "$manifest" "$app_name" 304 "$manifest_name" "--nodes=4 --ntasks=304 --cpus-per-task=1 --hint=nomultithread"
  wait_all
done

for manifest in "${rwd[@]}"; do
  manifest_name="$(basename "${manifest%.manifest}")"
  run_rwd "$app" "${app_name}_${manifest_name}_np532.json" "$manifest" "$app_name" 532 "$manifest_name" "--nodes=7 --ntasks=532 --cpus-per-task=1 --hint=nomultithread" 
done