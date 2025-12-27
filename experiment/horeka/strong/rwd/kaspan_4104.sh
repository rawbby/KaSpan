#!/bin/bash
#SBATCH --nodes=54
#SBATCH --ntasks=4104
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_4104.out
#SBATCH -e kaspan_4104.err
#SBATCH -J kaspan_4104
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh
app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan
rwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )
set +eu
for manifest in "${rwd[@]}"; do
manifest_name="$(basename "${manifest%.manifest}")"
output_file="${app_name}_${manifest_name}_np4104.json"
run_rwd "$app" "$output_file" "$manifest" "$app_name" 4104 "$manifest_name" "--nodes=54 --ntasks=4104 --cpus-per-task=1 --hint=nomultithread"
done
