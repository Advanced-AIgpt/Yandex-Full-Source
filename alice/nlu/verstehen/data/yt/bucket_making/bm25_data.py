import cPickle as pickle
import logging
from collections import defaultdict

import numpy as np

from verstehen.index.word_match import BM25Index
from .util import mkdir_for_file, row_iterator

logger = logging.getLogger(__name__)


def fetch_bm25_data(bm25_token_mapping_table, bm25_doc_lens_table, bm25_output_file, use_tqdm=False):
    mkdir_for_file(bm25_output_file)

    logger.debug('Fetching rows from table {}, extracting BM25 mapping'.format(bm25_token_mapping_table))

    token_id_mapping = defaultdict(dict)
    for row in row_iterator(bm25_token_mapping_table, in_parallel=True, use_tqdm=use_tqdm):
        token = row['token'].decode('utf-8')
        id = int(row['verstehen_id'])
        occurrences = int(row['occurrences'])
        token_id_mapping[token][id] = occurrences

    token_id_mapping = BM25Index.efficient_token_id_mapping(token_id_mapping)

    logger.debug('Fetching rows from table {}, extracting doc lens for BM25'.format(bm25_doc_lens_table))

    doc_lens = []
    for row in row_iterator(bm25_doc_lens_table, in_parallel=False, use_tqdm=use_tqdm):
        doc_lens.append(int(row['doc_len']))
    doc_lens = np.array(doc_lens)

    result = {
        'token_id_mapping': token_id_mapping,
        'doc_lens': doc_lens
    }

    logger.debug('Dumping BM25 pickle file to {}'.format(bm25_output_file))

    with open(bm25_output_file, 'w') as f:
        pickle.dump(result, f)
