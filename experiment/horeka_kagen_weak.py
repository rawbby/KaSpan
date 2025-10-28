#!/bin/python
import datetime
import math

from base import *
from gnm_directed import *
from horeka import Horeka
from ispan import *
from kaspan import *
from rmat_directed import *


def main():
    matrix: Dict[str, List[Any]] = {
        # competitors
        'program': [
            KaSpan(),
            KaSpan(True),
            KaSpan(True, True),
            ISpan()
        ],

        # data-variants
        'N': [16, 18, 20],
        'd': [0.5, 1.0, 2.0, 4.0, 8.0, 16.0],
        'graph': [
            # RmatDirected(0.57, 0.19, 0.19),
            # RmatDirected(0.25, 0.25, 0.25),
            GnmDirected()
        ],

        # x-axis
        'NP': [6, 8, 10, 12],
    }

    experiment = f"{now_hash()}_{seed_hash()}"
    experiment_dir = experiment_data_dir / experiment
    seed = random_seed()

    runs: Dict[str, Dict[str, Any]] = dict()

    for N, d, NP, program, graph in matrix_product(matrix, 'N', 'd', 'NP', 'program', 'graph'):

        n = 2 ** (NP + N)
        m = math.ceil(d * n)

        config: Dict[str, Any] = {
            'experiment': experiment,
            'experiment_dir': experiment_dir,
            'seed': seed,
            'n': n,
            'm': m,
            'np': 2 ** NP,
            'timeout': datetime.timedelta(minutes=30)
        }

        graph(config)
        program(config)

        config['job'] = experiment_dir / f"{config['run']}.sh"
        config['err'] = experiment_dir / f"{config['run']}.err"
        config['out'] = experiment_dir / f"{config['run']}.out"

        runs[config['run']] = config

    horeka = Horeka()
    horeka(runs)

if __name__ == '__main__':
    main()
