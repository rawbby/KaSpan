#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=76
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_single.out
#SBATCH -e kaspan_single.err
#SBATCH -J kaspan_single
#SBATCH --partition=cpuonly
#SBATCH --time=45:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh
app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan
rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )
set +eu
for manifest in "${rwd[@]}"; do
for np in 76 38 16 8 4 2 1; do
manifest_name="$(basename "${manifest%.manifest}")"
output_file="${app_name}_${manifest_name}_np${np}.json"
run_rwd "$app" "$output_file" "$manifest" "$app_name" "$np" "$manifest_name" "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread"
done
done
