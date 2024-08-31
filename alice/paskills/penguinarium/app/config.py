import yaml
import operator
from typing import Any

name2operator = {
    'lt': operator.lt,
    'le': operator.le,
    'gt': operator.gt,
    'ge': operator.ge
}

slow_response_bins = list(range(100, 10000, 500)) + [500000]
delayed_response_bins = [1, 10, 30, 50] + list(range(100, 5000, 300)) + [10000]
quick_response_bins = [1, 3, 5, 8, 10, 20, 40, 80, 150, 300, 500, 700, 1000, 2000, 5000]
fast_response_bins = [1, 2, 3, 5, 8, 12, 16, 20, 25, 40, 80, 100, 150, 200, 300, 500, 1000, 5000]

METRICS_CONF = {
    'nodes_storage_add_time': {'type': 'hist', 'bins': quick_response_bins},
    'nodes_storage_get_time': {'type': 'hist', 'bins': fast_response_bins},
    'ydb_get_time': {'type': 'hist', 'bins': fast_response_bins},
    'deserialization_time': {'type': 'hist', 'bins': fast_response_bins},
    'resolve_intents_time': {'type': 'hist', 'bins': fast_response_bins},
    'cache_warm_up': {'type': 'rate'},
    'ydb_errors': {'type': 'rate'},

    'http_requests': {'type': 'rate'},
    'http_response_time': {'type': 'hist', 'bins': fast_response_bins},
    'http_responses': {'type': 'rate'},
}


class PenguinariumConfig:
    def __init__(self, path_to_config: str) -> None:
        with open(path_to_config, 'r') as f:
            self._config = yaml.safe_load(f)

        operator_name = self._config['model']['dist_thresh_rel']
        self._config['model']['dist_thresh_rel'] = name2operator[operator_name]
        self._config['model'].setdefault('p', 2)
        self._config['metrics'] = METRICS_CONF

    def __getitem__(self, name: str) -> Any:
        return self._config[name]
