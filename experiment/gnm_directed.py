from typing import Dict, Any

__all__ = ['GnmDirected']


class GnmDirected:
    def __call__(self, config: Dict[str, Any]):
        assert 'n' in config
        assert 'm' in config
        assert 'seed' in config

        config['graph'] = 'gnm_directed_n{n}_m{m}_s{seed}'.format_map(config)
        options = config.setdefault('options', list())
        options.append('--kagen_option_string')
        options.append('"gnm-directed;n={n};m={m};seed={seed}"'.format_map(config))
