# -*- coding: utf-8 -*-

import click
import numpy as np
from collections import Counter
from dataset import VinsDataset


def _add_factor_to_dataset(dataset, mapping):
    factor = np.array([[mapping.get(info.prev_intent, 0)] for info in dataset._additional_infos])
    dataset.add_feature(
        feature_name='prev_intent',
        feature_type=VinsDataset.FeatureType.DENSE,
        feature_matrix=factor,
        feature_mapping=mapping
    )


@click.command()
@click.option('--train-data-path', type=click.Path(exists=True))
@click.option('--val-data-path', type=click.Path(exists=True))
def main(train_data_path, val_data_path):
    train_dataset = VinsDataset.restore(train_data_path)
    counter = Counter((x.prev_intent for x in train_dataset._additional_infos))

    mapping = {'unk': 0}
    for intent, count in counter.most_common():
        if count <= 2:
            break
        mapping[intent] = len(mapping)

    _add_factor_to_dataset(train_dataset, mapping)
    train_dataset.save(train_data_path)

    val_dataset = VinsDataset.restore(val_data_path)
    _add_factor_to_dataset(val_dataset, mapping)
    val_dataset.save(val_data_path)


if __name__ == '__main__':
    main()
