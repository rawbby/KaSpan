#!/bin/bash
#SBATCH --nodes=${NODES}
#SBATCH --ntasks=${NTASKS}
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-node=${NTASKS_PER_NODE}
#SBATCH -o ${OUT_FILE}
#SBATCH -e ${ERR_FILE}
#SBATCH -J ${JOB_NAME}
#SBATCH --partition=${PARTITION}
#SBATCH --time=${TIME}
#SBATCH --export=ALL
#SBATCH --mem=${MEM}

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30
source .venv/bin/activate

I_MPI_PIN=1               \
I_MPI_PIN_DOMAIN=core     \
I_MPI_PIN_ORDER=compact   \
I_MPI_JOB_TIMEOUT=${TIME} \
mpiexec.hydra             \
  -n ${NTASKS}            \
  -bootstrap slurm        \
  ${CMD}
