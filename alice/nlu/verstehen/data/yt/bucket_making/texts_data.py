import cPickle as pickle
import logging
from collections import defaultdict

from .util import mkdir_for_file, row_iterator

logger = logging.getLogger(__name__)


def fetch_texts_data(texts_table, texts_output_file, payload_output_file=None, use_tqdm=False):
    mkdir_for_file(texts_output_file)

    logger.debug('Fetching rows from table {}, extracting texts'.format(texts_table))

    columns_data = defaultdict(list)

    for row in row_iterator(texts_table, in_parallel=False, use_tqdm=use_tqdm):
        if 'text' not in row:
            raise ValueError('`text` column must be present in the table values.')

        for key, value in row.items():
            if key == 'verstehen_id':  # verstehen id is a system internal thing, not needed in final payload
                continue

            if type(value) == str:
                value = value.decode('utf-8')
            columns_data[key].append(value)

    logger.debug('Dumping pickle with texts to {}'.format(texts_output_file))

    with open(texts_output_file, 'w') as f:
        pickle.dump(columns_data['text'], f)

    columns_data = {key: value for key, value in columns_data.items() if key != 'text'}
    if len(columns_data) > 0:
        if payload_output_file is None:
            raise ValueError('If payload columns are present, payload output file must be specified!')

        logger.debug('Dumping pickle with payload columns to {}'.format(payload_output_file))
        with open(payload_output_file, 'w') as f:
            pickle.dump(columns_data, f)
