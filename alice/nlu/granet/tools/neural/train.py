# coding: utf-8

import argparse
import attr
import codecs
import json
import logging
import numpy as np
import os
import random
import re

from model import BatchGenerator, RnnTaggerTrainer
from utils import iterate_lines, parse_nlu_item, get_text_without_markup, load_embeddings

logger = logging.getLogger(__name__)


_PAD_LABELS = True


@attr.s
class Example(object):
    tokens = attr.ib()
    tags = attr.ib()
    tag_mask = attr.ib()
    class_label = attr.ib()
    token_ids = attr.ib(default=None)
    tag_ids = attr.ib(default=None)
    class_id = attr.ib(default=None)

    def __len__(self):
        return len(self.tokens)


def _read_data_file(path, class_label):
    assert class_label in {'positive', 'negative'}

    class_id = 1 if class_label == 'positive' else 0

    examples = []
    for weight, text in iterate_lines(path):
        tokens, tags = parse_nlu_item(text)
        assert len(tokens) == len(tags)

        cleaned_text = get_text_without_markup(text)
        if len(tokens) != len(cleaned_text.split()):
            logger.warning('Misalignment: %s  --->  %s', cleaned_text, ' '.join(tokens))

        if class_id == 0:
            # TODO: consider zeros-filled mask for negatives
            tags = ['O'] * len(tags)

        mask = [1] * len(tags)
        tags = [class_label] + tags

        examples.append(Example(
            tokens=tokens,
            tags=tags,
            tag_mask=mask,
            class_label=class_label,
            class_id=class_id
        ))

    return examples


def _collect_tag_indices(examples):
    shift = 1 if _PAD_LABELS else 0
    tags = {
        'negative': 0 + shift,
        'positive': 1 + shift,
    }
    for example in examples:
        for tag in example.tags:
            if tag not in tags:
                tags[tag] = len(tags) + shift
    return tags


def _index_data(examples, tag_to_index, word_to_index):
    for example in examples:
        example.token_ids = [word_to_index.get(token, 0) for token in example.tokens]
        example.tag_ids = [tag_to_index[tag] for tag in example.tags]


def _read_data(positives_path, negatives_path, word_to_index):
    positives = _read_data_file(
        path=positives_path,
        class_label='positive'
    )

    tag_to_index = _collect_tag_indices(positives)
    _index_data(positives, tag_to_index=tag_to_index, word_to_index=word_to_index)

    negatives = _read_data_file(
        path=negatives_path,
        class_label='negative'
    )
    _index_data(negatives, tag_to_index=tag_to_index, word_to_index=word_to_index)

    logger.info('Positives count: %s, negatives count: %s', len(positives), len(negatives))

    examples = positives + negatives

    random.shuffle(examples)

    features_mapping = {
        'tags': tag_to_index,
        'classes': {
            'negative': 0,
            'positive': 1
        }
    }

    return examples, features_mapping


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--positives-path', required=True)
    parser.add_argument('--negatives-path', required=True)
    parser.add_argument('--embeddings-dir', required=True)
    parser.add_argument('--output-path', default='models')

    args = parser.parse_args()

    logger.info('Loading embeddings...')
    embeddings_matrix, _, word_to_index = load_embeddings(args.embeddings_dir)

    logger.info('Reading data...')
    examples, features_mapping = _read_data(args.positives_path, args.negatives_path, word_to_index)

    logger.info('Features mappings:')
    for feature_type, mapping in features_mapping.iteritems():
        logger.info('Feature: %s', feature_type)
        for feature, index in sorted(mapping.iteritems(), key=lambda pair: pair[1]):
            logger.info('\t%s - %s', feature, index)

    logger.info('Examples:')
    for example in examples[0:1000:50]:
        logger.info(example)

    generator = BatchGenerator(examples, batch_size=128)

    logger.info('Batch: %s', next(generator))

    trainer = RnnTaggerTrainer(
        embeddings_matrix=embeddings_matrix, epoch_count=30, features_mapping=features_mapping
    )
    trainer.fit(generator)
    trainer.save(args.output_path)


if __name__ == "__main__":
    main()
