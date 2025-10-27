import datetime
import math
from typing import Any
from typing import Dict

__all__ = ['Horeka']

horeka_template = '''
#!/bin/bash
#SBATCH --nodes={slurm_nodes}
#SBATCH --ntasks={np}
#SBATCH --cpus-per-task=1
#SBATCH --ntasks-per-node={slurm_ntasks_per_node}
#SBATCH -o {out}
#SBATCH -e {err}
#SBATCH -J {experiment}_{run}
#SBATCH --partition=cpuonly
#SBATCH --time={slurm_timeout}
#SBATCH --export=ALL
#SBATCH --mem=230gb

module purge
module load compiler/gnu/14
module load mpi/impi/2021.11
module load devel/cmake/3.30

I_MPI_PIN=1                     \\
I_MPI_PIN_DOMAIN=core           \\
I_MPI_PIN_ORDER=compact         \\
I_MPI_JOB_TIMEOUT={mpi_timeout} \\
mpiexec.hydra                   \\
  -n {np}                       \\
  -bootstrap slurm              \\
  {cmd}
'''


class Horeka:
    def __init__(self, min_timeout=120, max_timeout=1800):
        self.min_timeout = min_timeout
        self.max_timeout = max_timeout

    def slurm_timeout(self, timeout: datetime.timedelta) -> str:
        """Round up to next minute, add one minute, return SLURM time 'D-HH:MM:SS'."""
        seconds = int(timeout.total_seconds())
        seconds = min(self.max_timeout, seconds)
        seconds = max(self.min_timeout, seconds)
        minutes = ((seconds + 59) // 60) + 1
        d, r = divmod(minutes, 24 * 60)
        h, m = divmod(r, 60)
        return f"{d:d}-{h:02d}:{m:02d}:00"

    def mpi_timeout(self, timeout: datetime.timedelta) -> str:
        """Round up to next second, add one second, return total_seconds."""
        seconds = math.ceil(timeout.total_seconds()) + 1
        seconds = min(self.max_timeout, seconds)
        seconds = max(self.min_timeout, seconds)
        return str(seconds)

    def __call__(self, config: Dict[str, Any]):
        assert 'timeout' in config and isinstance(config['timeout'], datetime.timedelta)
        assert 'np' in config
        assert 'job' in config
        assert 'err' in config
        assert 'out' in config
        assert 'experiment' in config
        assert 'run' in config
        assert 'exe' in config

        cmd = [config['exe']] + config.setdefault('options', list())
        cmd = ' '.join([str(arg) for arg in cmd])

        horeka_config = config.copy()
        horeka_config['mpi_timeout'] = self.mpi_timeout(config['timeout'])
        horeka_config['slurm_timeout'] = self.slurm_timeout(config['timeout'])
        horeka_config['slurm_nodes'] = (config['np'] + 71) // 72
        horeka_config['slurm_ntasks_per_node'] = min(72, config['np'])
        horeka_config['cmd'] = cmd

        config['experiment_dir'].mkdir(parents=True, exist_ok=True)
        config['job'].write_text(horeka_template.format_map(horeka_config))
