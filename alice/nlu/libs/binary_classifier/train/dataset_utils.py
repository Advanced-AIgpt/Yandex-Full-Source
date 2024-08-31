# coding: utf-8

import attr
import logging
import numpy as np
import yt.wrapper as yt

from tqdm import tqdm

logger = logging.getLogger(__name__)


@attr.s
class DatasetConfig(object):
    source = attr.ib(validator=attr.validators.in_(['yt', 'nlu']))
    intent = attr.ib()
    yt_path = attr.ib(default=None)
    negatives_to_remove = attr.ib(default=None)
    filter_full_textual_intersections = attr.ib(default=True)
    validation_split = attr.ib(default=0.1)


def _parse_row(row):
    parsed_row = {}
    for key, value in row.items():
        key = key.decode('utf8')
        if key == '_other':
            continue
        if 'embedding' in key:
            value = np.frombuffer(value, dtype=np.float32).reshape(300)
        elif not isinstance(value, bool):
            value = value.decode('utf8')
        parsed_row[key] = value
    return parsed_row


def _download_dataset(yt_params):
    yt.config['proxy']['url'] = yt_params['cluster']
    yt.config['read_parallel']['enable'] = True
    yt.config['read_parallel']['max_thread_count'] = 64

    total = yt.row_count(yt_params['table'])
    return [_parse_row(row) for row in tqdm(yt.read_table(yt_params['table']), total=total)]


def _filter_similar_negatives(dataset, negatives_to_remove):
    def _should_be_filtered(intent):
        return any(intent.endswith(intent_to_remove) for intent_to_remove in negatives_to_remove)

    return [sample for sample in dataset if not _should_be_filtered(sample['intent'])]


def _filter_full_textual_intersections(dataset, target_intents):
    target_intent_texts = {
        sample['utterance_text']
        for sample in dataset if sample['intent'] in target_intents
    }

    return [
        sample
        for sample in dataset
        if sample['intent'] in target_intents or sample['utterance_text'] not in target_intent_texts
    ]


def _add_target_label(dataset, target_intents):
    for sample in dataset:
        sample['label'] = int(sample['intent'] in target_intents)


def _split_dataset(dataset, validation_split):
    sample_indices = np.arange(len(dataset))
    np.random.shuffle(sample_indices)
    train_size = int(len(dataset) * (1. - validation_split))
    train_indices, valid_indices = sample_indices[:train_size], sample_indices[train_size:]

    train_dataset = [dataset[index] for index in train_indices]
    valid_dataset = [dataset[index] for index in valid_indices]

    return train_dataset, valid_dataset


def _prepare_embedding_dataset(dataset, embedding_name):
    embeddings = [sample[embedding_name] for sample in dataset]
    labels = [sample['label'] for sample in dataset]

    return np.array(embeddings), np.array(labels)


def load_dataset(config, yt_params, embedding_name):
    dataset = _download_dataset(yt_params)

    dataset = _filter_similar_negatives(dataset, config.negatives_to_remove)

    if config.filter_full_textual_intersections:
        dataset = _filter_full_textual_intersections(dataset, config.intent)

    _add_target_label(dataset, config.intent)

    train_dataset, valid_dataset = _split_dataset(dataset, config.validation_split)

    logger.info('Train dataset size = %s, positives count = %s',
                len(train_dataset), sum(sample['label'] for sample in train_dataset))
    logger.info('Valid dataset size = %s, positives count = %s',
                len(valid_dataset), sum(sample['label'] for sample in valid_dataset))

    dataset_size = len(train_dataset)
    for sample in train_dataset[0: dataset_size / 10: dataset_size]:
        logger.info(sample)

    train_data = _prepare_embedding_dataset(train_dataset, embedding_name)
    valid_data = _prepare_embedding_dataset(valid_dataset, embedding_name)

    return train_data, valid_data
