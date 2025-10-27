from pathlib import Path
from typing import Dict, Any, List

from base import binary_dir


class ISpan:
    def __init__(self, alpha: int = 12):
        self.alpha = alpha

    def __call__(self, config: Dict[str, Any]):
        assert 'experiment_dir' in config and isinstance(config['experiment_dir'], Path)
        assert 'graph' in config
        assert 'np' in config

        config['exe'] = binary_dir / 'bench_ispan'

        config['program'] = 'ispan'
        config['run'] = f"{config['program']}_np{config['np']}_{config['graph']}"
        config['result'] = config['experiment_dir'] / f"{config['run']}.json"

        options = config.setdefault('options', list())
        options.append('--alpha')
        options.append(self.alpha)
        options.append('--output')
        options.append(config['result'])

    def timings(self):
        result: List[str] = list()
        result.append('scc')
        result.append('scc.alloc')
        result.append('scc.forward_backward_search')
        # todo
        return result
