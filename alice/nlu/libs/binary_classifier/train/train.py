# coding: utf-8

import attr
import click
import json
import os
import logging

from dataset_utils import DatasetConfig, load_dataset
from dssm_based_binary_classifier import BinaryClassifier, ModelConfig

try:
    import nirvana_dl
except ImportError:
    nirvana_dl = None

logger = logging.getLogger(__name__)


@attr.s
class TrainConfig(object):
    dataset = attr.ib(validator=attr.validators.instance_of(DatasetConfig))
    model = attr.ib(validator=attr.validators.instance_of(ModelConfig))

    @classmethod
    def from_json(cls, json_path):
        with open(json_path) as f:
            config = json.load(f)
        assert 'dataset' in config and 'model' in config, 'Wrong config format'

        return cls(
            dataset=DatasetConfig(**config['dataset']),
            model=ModelConfig(**config['model']),
        )


@click.command()
@click.option('--config-path', type=click.Path(exists=True), required=True)
@click.option('--output-dir', required=True)
@click.option('--input-table', default=None)
def main(config_path, output_dir, input_table):
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    params = {
        'table': input_table,
        'cluster': 'hahn',
    }
    if nirvana_dl is not None:
        params.update(nirvana_dl.params())

    config = TrainConfig.from_json(config_path)

    train_data, valid_data = load_dataset(
        config=config.dataset,
        yt_params=params,
        embedding_name=config.model.embedding
    )

    model = BinaryClassifier(config=config.model, is_training=True)

    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    model.fit(train_data, valid_data, output_dir)
    model.save(output_dir, save_to_protobuf_format=True)


if __name__ == "__main__":
    main()
