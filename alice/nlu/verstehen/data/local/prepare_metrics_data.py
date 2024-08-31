import argparse
import logging
import os
import re
from collections import defaultdict

from verstehen.config.data_config import MetricsDataConfig
from verstehen.preprocess import filter_duplicates_by_lower_case
from .util import json_read, json_dump, read_tsv_columns

logger = logging.getLogger(__name__)


def process_toloka_tsv(toloka_tsv_path, toloka_intent_renames_path, out_file_path):
    logger.debug('Starting processing toloka TSV file from path {}'.format(toloka_tsv_path))

    logger.debug('Reading toloka intents renaming from {}'.format(toloka_intent_renames_path))
    toloka_intents_renames = json_read(toloka_intent_renames_path)['true_intent']

    intents, utterances = read_tsv_columns(toloka_tsv_path, column_ids=[0, 1])

    toloka_intents = set(intents)
    toloka_to_vins_intents = {}

    for intent in toloka_intents:
        for regular_expression in toloka_intents_renames:
            if re.match(regular_expression, intent) is not None:
                toloka_to_vins_intents[intent] = toloka_intents_renames[regular_expression]
                break

    intents_to_texts_toloka = defaultdict(list)
    for toloka_intent, utterance in zip(intents, utterances):
        if toloka_intent not in toloka_to_vins_intents:
            continue
        vins_intent = toloka_to_vins_intents[toloka_intent]
        intents_to_texts_toloka[vins_intent].append(utterance)

    directory = os.path.dirname(out_file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)

    logger.debug('Dumping mapped intents and texts to {}'.format(out_file_path))
    json_dump(intents_to_texts_toloka, out_file_path)


def filter_by_intents(intents_to_texts, remain_intents):
    filtered_map = dict()

    for intent, texts in intents_to_texts.items():
        if intent not in remain_intents:
            continue
        filtered_map[intent] = texts

    return filtered_map


def filter_texts(intents_to_texts, texts_filtering_fn):
    filtered_intents_to_texts = dict()
    for intent, texts in intents_to_texts.items():
        filtered_intents_to_texts[intent] = texts_filtering_fn(texts, processes=10)
    return filtered_intents_to_texts


def prepare_data(index_data, queries_data, out_index_data_path, out_queries_data_path, texts_filtering_fn=None):
    logger.debug('Filtering data by intents')
    intents_to_remain = set(index_data.keys())
    index_data = filter_by_intents(index_data, remain_intents=intents_to_remain)
    queries_data = filter_by_intents(queries_data, remain_intents=intents_to_remain)

    if texts_filtering_fn is not None:
        logger.debug('Filtering texts with filtering function')
        index_data = filter_texts(index_data, texts_filtering_fn)
        queries_data = filter_texts(queries_data, texts_filtering_fn)

    logger.debug('Dumping data')
    json_dump(index_data, out_index_data_path)
    json_dump(queries_data, out_queries_data_path)


def configure_parser(parser=None):
    # Configuring argument parser with optional creation of a new one. Handy in case of a subparser need
    if parser is None:
        parser = argparse.ArgumentParser()

    parser.add_argument(
        '--toloka_intent_renames_path', type=str,
        default=MetricsDataConfig.TOLOKA_INTENT_RENAMES_PATH,
        help='Path to mapping of Toloka intent names to vins intent names'
    )
    parser.add_argument(
        '--toloka_tsv_path', type=str,
        default=MetricsDataConfig.TOLOKA_TSV_PATH,
        help='Source TSV file with toloka daily dataset. Expected columns: `intent` and `text`'
    )
    parser.add_argument(
        '--toloka_intents_to_texts_path', type=str,
        default=MetricsDataConfig.TOLOKA_INTENT_TO_TEXTS_PATH,
        help='Path to the file with JSON map of intent to texts. If not exists, will be created from `toloka_tsv_path`.'
    )
    parser.add_argument(
        '--nlu_intents_to_texts_path', type=str,
        default=MetricsDataConfig.NLU_INTENT_TO_TEXTS_PATH,
        help='Path to the file with JSON map of intent to texts.'
    )
    parser.add_argument(
        '--out_index_data_path', type=str,
        default=MetricsDataConfig.PREPARED_INDEX_DATA_PATH,
        help='Path of prepared index data. Will be JSON map of intent to associated texts.'
    )
    parser.add_argument(
        '--out_queries_data_path', type=str,
        default=MetricsDataConfig.PREPARED_QUERIES_DATA_PATH,
        help='Path of prepared queries data. Will be JSON map of intent to associated texts.'
    )
    return parser


def main(args=None):
    if not os.path.exists(args.toloka_intents_to_texts_path):
        process_toloka_tsv(args.toloka_tsv_path, args.toloka_intent_renames_path, args.toloka_intents_to_texts_path)

    logger.debug('Reading source data for index and queries')
    index_data = json_read(args.toloka_intents_to_texts_path)
    queries_data = json_read(args.nlu_intents_to_texts_path)

    prepare_data(index_data, queries_data,
                 out_index_data_path=args.out_index_data_path,
                 out_queries_data_path=args.out_queries_data_path,
                 texts_filtering_fn=filter_duplicates_by_lower_case)


if __name__ == '__main__':
    args = configure_parser().parse_args()
    main(args)
