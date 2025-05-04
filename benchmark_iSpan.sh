#!/usr/bin/env bash
#SBATCH --job-name=benchmark_iSpan
#SBATCH --output=mpi_benchmark_%j.out
#SBATCH --error=mpi_benchmark_%j.err
#SBATCH --ntasks=32               # Total number of MPI tasks
#SBATCH --nodes=2                 # Number of nodes
#SBATCH --ntasks-per-node=16      # Tasks per node (adjust for your CPU/node config)
#SBATCH --time=01:00:00           # Max run time
#SBATCH --partition=compute       # Partition to submit to (adjust as needed)

set -euo pipefail

dd="$(dirname "$0")"
cd "$dd"

echo "activating local spack environment"
spack env activate --create --dir .

export I_MPI_DEBUG=5
export I_MPI_PIN=1
export I_MPI_FABRICS=shm:ofi

mpirun -n $SLURM_NTASKS ./bin/iSpan
