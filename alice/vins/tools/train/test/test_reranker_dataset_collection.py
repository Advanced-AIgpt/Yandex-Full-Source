# -*- coding: utf-8 -*-

import os
import pytest
import subprocess
import shutil
import tempfile

from itertools import izip
from ..reranker_models.reranker_model import RerankerModel


def _check_feature_description(tempdir):
    expected_header = ['0\tLabel\n', '1\tGroupId\n', '2\tAuxiliary\tQuery text\n',
                       '3\tAuxiliary\tTrue label\n', '4\tAuxiliary\tPredicted label\n']

    expected_features = RerankerModel.get_features()

    with open(os.path.join(tempdir, 'train.cd')) as f:
        feature_descriptions = list(f)
        assert len(feature_descriptions) == len(expected_header) + len(expected_features)

        for feature_description, expected_feature_description in izip(feature_descriptions, expected_header):
            assert feature_description == expected_feature_description

        feature_descriptions = feature_descriptions[len(expected_header):]
        for feature_description, expected_feature in izip(feature_descriptions, expected_features):
            feature = feature_description.strip().split('\t')[2]
            assert feature == expected_feature

        return len(expected_header) + len(expected_features)


@pytest.mark.slowtest
def test_dataset_collection():
    tempdir = None
    try:
        tempdir = tempfile.mkdtemp()

        subprocess.check_call([
            'tools/train/collect_reranker_dataset.sh',
            'tools/train/test/test_data/train_data_sample.tsv',
            os.path.join(tempdir, 'train_data_sample.tar'),
            'tools/train/test/test_data/test_data_sample.tsv',
            os.path.join(tempdir, 'test_data_sample.tar'),
            tempdir,
            '1'
        ])

        assert os.path.isfile(os.path.join(tempdir, 'train.tsv'))
        assert os.path.isfile(os.path.join(tempdir, 'train.cd'))
        assert os.path.isfile(os.path.join(tempdir, 'val.tsv'))

        features_count = _check_feature_description(tempdir)

        with open(os.path.join(tempdir, 'train.tsv')) as f:
            assert len(f.readline().strip().split('\t')) == features_count

        with open(os.path.join(tempdir, 'val.tsv')) as f:
            assert len(f.readline().strip().split('\t')) == features_count
    finally:
        if tempdir:
            shutil.rmtree(tempdir)
