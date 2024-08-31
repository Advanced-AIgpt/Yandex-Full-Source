import logging
from typing import List, Dict

import attr
import numpy as np

from beggins.lib import config

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class Dataset:
    features = attr.ib()
    target = attr.ib()

    @classmethod
    def from_npz(cls, filename):
        data = np.load(filename)
        return cls(features=data['x'], target=data['y'])

    @classmethod
    def from_source(cls, dataset_config: config.DatasetSource):
        if dataset_config.type == 'npz':
            return cls.from_npz(dataset_config.filename)
        raise ValueError(f'unknown dataset type: {dataset_config.type}')


DatasetRegistry = Dict[str, Dataset]


def load_datasets(configs: List[config.Dataset]) -> DatasetRegistry:
    logger.info('Loading datasets...')
    datasets = {}
    for cfg in configs:
        datasets[cfg.name] = Dataset.from_source(cfg.source)
    logger.info(f'Loaded datasets: {list(datasets.keys())}')
    return datasets
