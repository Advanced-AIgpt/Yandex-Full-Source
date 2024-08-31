# -*- coding: utf-8 -*-

import click
from tqdm import tqdm
import mmap
import numpy as np

from codecs import open
from itertools import izip


def _get_num_lines(file_path):
    fp = open(file_path, "r+")
    buf = mmap.mmap(fp.fileno(), 0)
    lines = 0
    while buf.readline():
        lines += 1
    return lines


def _iterate_groups(data_path, feature_indices, group_id_index=1):
    cur_group_id, group, group_features = None, [], []
    with open(data_path, encoding='utf-8') as f:
        for line in tqdm(f, total=_get_num_lines(data_path)):
            line = line.strip()
            group.append(line)

            fields = line.split('\t')
            group_id = fields[group_id_index]
            if group_id != cur_group_id:
                if group_features:
                    yield group, np.array(group_features)
                cur_group_id, group, group_features = group_id, [], []

            group_features.append(tuple(float(fields[index]) for index in feature_indices))

    if group_features:
        yield group, np.array(group_features)


def _calc_group_meta_features(group_features):
    maxes = group_features.max(0)

    deltas = group_features - maxes
    ratios = group_features / (maxes + 1e-10)
    sorted_positions = np.argsort(-group_features, 0)

    return np.concatenate((deltas, ratios, sorted_positions), -1)


@click.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--data-output-path', default=None)
@click.option('--cd-path', type=click.Path(exists=True))
@click.option('--cd-output-path', default=None)
@click.option('--feature-indices')
def main(data_path, data_output_path, cd_path, cd_output_path, feature_indices):
    feature_indices = [int(index) for index in feature_indices.split(',')]

    cd_output_path = cd_output_path or (cd_path + '_')
    with open(cd_output_path, 'w') as f_out:
        feature_names = []
        feature_index = 0
        with open(cd_path) as f:
            for index, line in enumerate(f):
                feature_index = index
                f_out.write(line)

                if index in feature_indices:
                    feature_names.append(line.strip().split('\t')[2])

        for meta_feature_type in ['delta', 'ratio', 'position']:
            for feature_name in feature_names:
                feature_index += 1
                f_out.write('{}\t{}\t{}_{}\n'.format(feature_index, 'Num', feature_name, meta_feature_type))

    data_output_path = data_output_path or (data_path + '_')
    with open(data_output_path, 'w', encoding='utf-8') as f_out:
        for group, group_features in _iterate_groups(data_path, feature_indices):
            group_meta_features = _calc_group_meta_features(group_features)

            for old_features, meta_features in izip(group, group_meta_features):
                old_features += '\t' + '\t'.join('{:g}'.format(feature) for feature in meta_features) + '\n'
                f_out.write(old_features)


if __name__ == '__main__':
    main()
