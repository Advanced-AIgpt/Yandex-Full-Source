# coding: utf-8

import json
import alice.nlu.tools.tagger.lib.sample_features as sf
import yt.wrapper as yt
from collections import defaultdict


def _load_raw_data_from_disk(path):
    with open(path) as fin:
        for line in fin:
            yield json.loads(line)


def _load_raw_data_from_yt(table):
    for row in yt.read_table(table):
        yield row


def _load_data(raw_data):
    intent_to_sample_features = defaultdict(list)
    for row in raw_data:
        sample_features = sf.SampleFeaturesWrapper.from_json(row['sample_features'])

        intent = row['intent']
        count = row.get('count', 1)

        for _ in range(count):
            intent_to_sample_features[intent].append(sample_features)

    return intent_to_sample_features


def load_data_from_disk(path):
    return _load_data(_load_raw_data_from_disk(path))


def load_data_from_yt(table):
    return _load_data(_load_raw_data_from_yt(table))
