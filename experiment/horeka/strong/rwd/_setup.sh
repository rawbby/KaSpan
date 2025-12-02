set -euo pipefail

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

source ~/workspace/KaSpan/.venv/bin/activate

source "~/workspace/KaSpan/experiment/horeka/strong/_${app_name}.sh"

export I_MPI_HYDRA_BOOTSTRAP=slurm
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi2.so
export I_MPI_PIN=1
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
export I_MPI_JOB_TIMEOUT=4

set +eu
