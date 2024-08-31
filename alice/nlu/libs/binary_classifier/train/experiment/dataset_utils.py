# coding: utf-8

import attr
import logging
import numpy as np
import re
import yt.wrapper as yt

from sklearn.feature_extraction.text import CountVectorizer, TfidfVectorizer
from tqdm import tqdm

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class DatasetConfig(object):
    train_path = attr.ib()
    valid_path = attr.ib()
    intent = attr.ib()
    negatives_to_remove = attr.ib()
    filter_full_textual_intersections = attr.ib()


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


def download_dataset(table_path):
    yt.config["read_parallel"]["enable"] = True
    yt.config["read_parallel"]["max_thread_count"] = 32

    row_count = yt.row_count(table_path)

    return [
        _parse_row(row)
        for row in tqdm(yt.read_table(table_path), total=row_count, desc='Loading: {}'.format(table_path))
    ]


def _filter_similar_negatives(dataset, negatives_to_remove):
    def _should_be_filtered(intent):
        return any(intent.endswith(intent_to_remove) for intent_to_remove in negatives_to_remove)

    return [
        sample for sample in dataset if not _should_be_filtered(sample['intent'])
    ]


def _filter_full_textual_intersections(dataset, target_intent):
    target_intent_texts = {
        sample['utterance_text']
        for sample in dataset if sample['intent'] == target_intent
    }

    return [
        sample
        for sample in dataset
        if sample['intent'] == target_intent or sample['utterance_text'] not in target_intent_texts
    ]


def _add_target_label(dataset, target_intent):
    for sample in dataset:
        sample['label'] = int(sample['intent'] == target_intent)


def load_dataset(config):
    train_dataset = download_dataset(config.train_path)
    valid_dataset = download_dataset(config.valid_path)

    train_dataset = _filter_similar_negatives(train_dataset, config.negatives_to_remove)
    valid_dataset = _filter_similar_negatives(valid_dataset, config.negatives_to_remove)

    if config.filter_full_textual_intersections:
        train_dataset = _filter_full_textual_intersections(train_dataset, config.intent)
        valid_dataset = _filter_full_textual_intersections(valid_dataset, config.intent)

    _add_target_label(train_dataset, config.intent)
    _add_target_label(valid_dataset, config.intent)

    logger.info('Train dataset size = %s, positives count = %s',
        len(train_dataset), sum(sample['label'] for sample in train_dataset)
    )
    logger.info('Valid dataset size = %s, positives count = %s',
        len(valid_dataset), sum(sample['label'] for sample in valid_dataset)
    )

    dataset_size = len(train_dataset)
    for sample in train_dataset[0: dataset_size / 10 : dataset_size]:
        logger.info(sample)

    return train_dataset, valid_dataset


def _prepare_text_dataset(dataset):
    texts = [sample['utterance_text'] for sample in dataset]
    labels = [sample['label'] for sample in dataset]

    return texts, np.array(labels)


def _prepare_embedding_dataset(dataset, embedding_name):
    embeddings = [sample[embedding_name] for sample in dataset]
    labels = [sample['label'] for sample in dataset]

    return np.array(embeddings), np.array(labels)


def _create_sklearn_vectorizer(vectorizer_params):
    vectorizer_type = vectorizer_params['type']
    del vectorizer_params['source']
    del vectorizer_params['type']

    if vectorizer_type == 'count':
        return CountVectorizer(**vectorizer_params)
    if vectorizer_type == 'tfidf':
        return TfidfVectorizer(**vectorizer_params)

    assert False, 'Unknown vectorizer type {}'.format(vectorizer_type)


def prepare_sklearn_data(train_dataset, valid_dataset, vectorizer_params):
    vectorizer = _create_sklearn_vectorizer(vectorizer_params)

    train_texts, train_labels = _prepare_text_dataset(train_dataset)
    valid_texts, valid_labels = _prepare_text_dataset(valid_dataset)

    vectorizer.fit(train_texts)
    train_vectors = vectorizer.transform(train_texts)
    valid_vectors = vectorizer.transform(valid_texts)

    return train_vectors, train_labels, valid_vectors, valid_labels, vectorizer


def prepare_dssm_data(train_dataset, valid_dataset, vectorizer_params):
    train_vectors, train_labels = _prepare_embedding_dataset(
        train_dataset, vectorizer_params['type']
    )
    valid_vectors, valid_labels = _prepare_embedding_dataset(
        valid_dataset, vectorizer_params['type']
    )
    vectorizer = None

    return train_vectors, train_labels, valid_vectors, valid_labels, vectorizer


def prepare_data(train_dataset, valid_dataset, vectorizer_params):
    if vectorizer_params['source'] == 'sklearn':
        return prepare_sklearn_data(train_dataset, valid_dataset, vectorizer_params)
    elif vectorizer_params['source'] == 'dssm':
        return prepare_dssm_data(train_dataset, valid_dataset, vectorizer_params)

    assert False, 'Unknown vectorizer source {}'.format(vectorizer_params['source'])
