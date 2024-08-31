import logging

import yt.wrapper as yt

from .util import mkdir_for_file, row_iterator

logger = logging.getLogger(__name__)


def fetch_dssm_knn_data(dssm_knn_embeddings_table, dssm_knn_model_file, dssm_knn_embeddings_output_file,
                        dssm_knn_model_output_file, use_tqdm=False):
    mkdir_for_file(dssm_knn_embeddings_output_file)
    mkdir_for_file(dssm_knn_model_output_file)

    logger.debug('Fetching rows from {} to extract embeddings and simultaneously write them as bytes to {}'.format(
        dssm_knn_embeddings_table, dssm_knn_embeddings_output_file
    ))

    with open(dssm_knn_embeddings_output_file, 'w') as f:
        for row in row_iterator(dssm_knn_embeddings_table, in_parallel=False, use_tqdm=use_tqdm):
            f.write(row['embedding'])

    logger.debug('Fetching YT file {} and saving it chunk by chunk to {}'.format(
        dssm_knn_model_file, dssm_knn_model_output_file
    ))

    yt.config.set_proxy('hahn.yt.yandex.net')

    with open(dssm_knn_model_file, 'rb') as from_file:
        with open(dssm_knn_model_output_file, 'wb') as to_file:
            while True:
                data = from_file.read(4096)
                if not data:
                    break
                to_file.write(data)
