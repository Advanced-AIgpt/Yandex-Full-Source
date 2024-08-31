# coding: utf-8

import numpy as np
import os

from alice.nlu.tools.binary_classifier.model import BinaryClassifier


def _collect_train_data(config, datasets):
    dataset_names = config.get('datasets', [])

    sentence_vectors = []
    labels = []
    for name in dataset_names:
        for target in ['positive', 'negative']:
            for selection in datasets[name][target]:
                if not selection['indexes']:
                    continue
                sentence_vectors.append(selection['sentence_vectors'])
                labels.append(selection['labels'])
    return {
        'sentence_vectors': np.concatenate(sentence_vectors),
        'labels': np.concatenate(labels),
    }


def _split_train_data(config, data):
    size = len(data['labels'])

    permutation = np.random.permutation(size)
    data = {k: v[permutation] for k, v in data.items()}

    split_pos = int(size * config.get('validation_split', 0))
    train = {k: v[split_pos:] for k, v in data.items()}
    valid = {k: v[:split_pos] for k, v in data.items()}

    return train, valid


def _print_train_data_info(name, data):
    total = len(data['labels'])
    positive = sum(data['labels'])
    print('  %s data: positive: %d, negative: %d, total: %d' % (name, positive, total - positive, total))


def _create_train_data(config, datasets):
    np.random.seed(config.get('seed', 0))
    data = _collect_train_data(config, datasets)
    train, valid = _split_train_data(config, data)
    _print_train_data_info('Train', train)
    _print_train_data_info('Valid', valid)
    return train, valid


def train_model(config, input_description, datasets):
    print('Train:')

    model_config = config['model']
    model_dir = model_config['model_dir']
    os.makedirs(model_dir, exist_ok=True)

    model = BinaryClassifier(model_config, input_description, is_training=True)

    train_config = config.get('train', {})
    train_data, valid_data = _create_train_data(train_config, datasets)
    model.fit(train_config['epoch_count'], train_data, valid_data)

    model.save(model_dir, save_to_protobuf_format=True)
    return model
