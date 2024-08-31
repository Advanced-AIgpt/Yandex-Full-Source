# -*- coding: utf-8 -*-

import os
import click
import numpy as np

from dataset import VinsDataset


def _calc_bag_of_entity_feature(dataset, mapping):
    feature = np.zeros((len(dataset), len(mapping)), dtype=np.bool)
    for sample_index, sample_info in enumerate(dataset._additional_infos):
        for entity_info in sample_info.entities_info:
            if entity_info in mapping:
                feature[sample_index, mapping[entity_info]] = True
            else:
                print 'Unknown entity_info:', entity_info

    return feature


@click.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--mappings-dir', default='apps/personal_assistant/personal_assistant/data/post_classifier')
def main(data_path, mappings_dir):
    mapping_path = os.path.join(mappings_dir, 'entitysearch_entities.txt')

    with open(mapping_path) as f:
        mapping = {line.rstrip(): index for index, line in enumerate(f)}

    dataset = VinsDataset.restore(data_path)

    entity_factor = _calc_bag_of_entity_feature(dataset, mapping)
    dataset.add_feature('entity', VinsDataset.FeatureType.DENSE, entity_factor, mapping)
    dataset.save(data_path)


if __name__ == '__main__':
    main()
