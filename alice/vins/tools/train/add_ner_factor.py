# -*- coding: utf-8 -*-

import os
import click
import numpy as np
from itertools import izip
from dataset import VinsDataset


def _add_ner_feature(path, mapping, ner_type):
    dataset = VinsDataset.restore(path)

    dataset_ner_mapping = dataset._sparse_feature_mappings[ner_type]
    dataset_index_to_ner = [ner for ner, _ in sorted(dataset_ner_mapping.iteritems(), key=lambda x: x[1])]
    ner_feature = dataset._sparse_embeddings[ner_type].tocoo()

    feature = np.zeros((ner_feature.shape[0], len(mapping)), dtype=np.bool)
    for token_ind, ner_ind in izip(ner_feature.row, ner_feature.col):
        if dataset_index_to_ner[ner_ind][2:] not in mapping:
            print dataset_index_to_ner[ner_ind][2:]
        else:
            feature[token_ind, mapping[dataset_index_to_ner[ner_ind][2:]]] = True

    reduced_feature = np.zeros((len(dataset._sample_ranges), len(mapping)), np.bool)
    for sample_ind, sample_range in enumerate(dataset._sample_ranges):
        reduced_feature[sample_ind] = feature[sample_range[0]: sample_range[1]].sum(axis=0)

    dataset.add_feature(
        feature_name=ner_type + '_feature',
        feature_type=VinsDataset.FeatureType.DENSE,
        feature_matrix=reduced_feature,
        feature_mapping=mapping
    )
    dataset.save(path)


@click.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--ner-type', type=click.Choice(['ner', 'wizard']))
@click.option('--mappings-dir', default='apps/personal_assistant/personal_assistant/data/post_classifier')
def main(data_path, ner_type, mappings_dir):
    mapping_path = os.path.join(mappings_dir, ner_type + '_entities.txt')

    with open(mapping_path) as f:
        mapping = {line.rstrip(): index for index, line in enumerate(f)}

    _add_ner_feature(data_path, mapping, ner_type)


if __name__ == '__main__':
    main()
