I_MPI_PIN=1                       \
I_MPI_PIN_DOMAIN=core             \
I_MPI_PIN_ORDER=compact           \
I_MPI_JOB_TIMEOUT=${TIME_SECONDS} \
mpiexec.hydra                     \
  -n ${NTASKS}                    \
  -bootstrap slurm                \
  ${CMD}
