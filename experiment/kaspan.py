from pathlib import Path
from typing import Dict, Any, List

from base import binary_dir


class KaSpan:
    def __init__(self, asynchronous: bool = False, asynchronous_indirection: bool = False):
        assert not asynchronous_indirection or asynchronous
        self.asynchronous = asynchronous
        self.asynchronous_indirection = asynchronous_indirection

    def __call__(self, config: Dict[str, Any]):
        assert 'experiment_dir' in config and isinstance(config['experiment_dir'], Path)
        assert 'graph' in config
        assert 'np' in config

        config['program'] = 'kaspan'
        config['exe'] = binary_dir / 'bench_kaspan'
        options = config.setdefault('options', list())

        if self.asynchronous:
            config['program'] += '_async'
            options.append('--async')
        if self.asynchronous_indirection:
            config['program'] += '_indirect'
            options.append('--async_indirect')

        config['run'] = f"{config['program']}_np{config['np']}_{config['graph']}"
        config['result'] = config['experiment_dir'] / f"{config['run']}.json"

        options.append('--output_file')
        options.append(f"\"{config['result']}\"")

    def metricies(self):
        result: List[str] = list()
        result.append('scc.duration')
        result.append('scc.alloc.duration')
        result.append('scc.forward_backward_search.duration')
        if self.asynchronous:
            result.append('')
        # todo
        return result
