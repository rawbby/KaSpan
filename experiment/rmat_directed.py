import math
from typing import Any
from typing import Dict

__all__ = ['RmatDirected']


class RmatDirected:
    def __init__(self, a: float, b: float, c: float):
        self.a = str(math.ceil(a * 100.0))
        self.b = str(math.ceil(b * 100.0))
        self.c = str(math.ceil(c * 100.0))
        if a == b == c:
            self.abc = f"abc{self.a}"
        if a == b:
            self.abc = f"ab{self.a}c{self.c}"
        if a == c:
            self.abc = f"ac{self.a}b{self.b}"
        if b == c:
            self.abc = f"a{self.a}bc{self.b}"
        else:
            self.abc = f"a{self.a}b{self.b}c{self.c}"

    def __call__(self, config: Dict[str, Any]):
        assert 'n' in config
        assert 'm' in config
        assert 'seed' in config

        config['a'] = float(self.a) / 100.0
        config['b'] = float(self.b) / 100.0
        config['c'] = float(self.c) / 100.0
        config['abc'] = self.abc

        config['graph'] = 'rmat_directed_{abc}_n{n}_m{m}_s{seed}'.format_map(config)
        options = config.setdefault('options', list())
        options.append('--kagen_option_string')
        options.append('gnm-directed;n={n};m={m};seed={seed}'.format_map(config))
