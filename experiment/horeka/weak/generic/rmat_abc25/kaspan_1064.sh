#!/bin/bash
#SBATCH --nodes=14
#SBATCH --ntasks=1064
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-socket=38
#SBATCH --ntasks-per-node=76
#SBATCH -o kaspan_1064.out
#SBATCH -e kaspan_1064.err
#SBATCH -J kaspan_1064
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh
app_name=kaspan
app=~/workspace/KaSpan/cmake-build-release/bin/bench_kaspan
set +eu
np=1064
for local_n in 150000 300000 600000; do
for d in 90 100 200 400; do
n=$(( np * local_n ))
m=$(( n * d / 100 ))
kagen_string="rmat;directed;n=${n};m=${m};a=0.25;b=0.25;c=0.25;seed=13"
output_file="${app_name}_rmat_abc25_np${np}_n${local_n}_d${d}.json"
run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "rmat_abc25" "--nodes=14 --ntasks=1064 --cpus-per-task=1 --hint=nomultithread"
done
done
