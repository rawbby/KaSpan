#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=76
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o isapn_single.out
#SBATCH -e isapn_single.err
#SBATCH -J ispan_single
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
app_name=ispan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_ispan
set +eu
for n in 1000000000 100000000 10000000; do
for d in 90 100 200 400; do
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13"
output_file="${app_name}_rmat_a59_bc19_np${np}_n${n}_d${d}.json"
run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "rmat_a59_bc19" "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread"
done
done
done
