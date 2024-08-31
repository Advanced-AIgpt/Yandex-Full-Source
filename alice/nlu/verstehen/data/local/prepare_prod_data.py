import argparse
import logging

from verstehen.config import LocalProductionDataConfig
from verstehen.preprocess import filter_duplicates_by_lemmatized
from .util import read_tsv_columns, json_dump

logger = logging.getLogger(__name__)


def configure_parser(parser=None):
    # Configuring argument parser with optional creation of a new one. Handy in case of a subparser need
    if parser is None:
        parser = argparse.ArgumentParser()

    parser.add_argument(
        '--tsv_utterance_source', type=str,
        default=LocalProductionDataConfig.BIG_TSV_SOURCE_PATH,
        help='Path to TSV source file that where the first column is utterance that will be used in index.'
    )
    parser.add_argument(
        '--out_file_path', type=str,
        default=LocalProductionDataConfig.BIG_PREPARED_DATA_PATH,
        help='Path to the result json file.'
    )
    return parser


def main(args=None):
    logger.debug('Reading utterance texts from TSV from path: {}'.format(args.tsv_utterance_source))
    utterance_texts = read_tsv_columns(args.tsv_utterance_source, column_ids=[0])

    logger.debug('Read {} texts from TSV file'.format(len(utterance_texts)))
    logger.debug('Applying filtering for duplicates')
    filtered = filter_duplicates_by_lemmatized(utterance_texts, processes=32)

    logger.debug('Dumping filtered {} texts to out file {}'.format(len(filtered), args.out_file_path))
    json_dump(filtered, args.out_file_path)


if __name__ == '__main__':
    args = configure_parser().parse_args()
    main(args)
