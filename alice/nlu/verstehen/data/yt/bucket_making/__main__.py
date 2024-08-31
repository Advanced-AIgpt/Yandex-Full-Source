import argparse
import logging
import os
import tarfile
import tempfile

from verstehen.config import YTProductionDataConfig
from verstehen.data.yt.bucket_making.bm25_data import fetch_bm25_data
from verstehen.data.yt.bucket_making.dssm_knn_data import fetch_dssm_knn_data
from verstehen.data.yt.bucket_making.texts_data import fetch_texts_data

logger = logging.getLogger(__name__)

if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s %(levelname)s:%(name)s %(message)s', level=logging.DEBUG)

    parser = argparse.ArgumentParser()

    # Texts table
    parser.add_argument(
        '--texts_table', type=str, default=YTProductionDataConfig.YT_TEXTS_TABLE
    )

    # BM25 tables
    parser.add_argument(
        '--bm25_token_id_mapping_table', type=str, default=YTProductionDataConfig.YT_BM25_TOKEN_ID_MAPPING_TABLE,
    )
    parser.add_argument(
        '--bm25_doc_lens_table', type=str, default=YTProductionDataConfig.YT_BM25_DOC_LENS_TABLE,
    )

    # DSSM KNN table and model
    parser.add_argument(
        '--dssm_knn_embeddings_table', type=str, default=YTProductionDataConfig.YT_DSSM_KNN_EMBEDDINGS_TABLE,
    )
    parser.add_argument(
        '--dssm_knn_model_file', type=str, default=YTProductionDataConfig.YT_DSSM_KNN_MODEL_FILE,
    )
    parser.add_argument(
        '--tempfolder', type=str, default=None
    )
    parser.add_argument(
        '--output_file', type=str, required=True
    )

    # Output files to write
    parser.add_argument(
        '--texts_output_filename', type=str, default=YTProductionDataConfig.TEXTS_FILENAME
    )
    parser.add_argument(
        '--payload_output_filename', type=str, default=YTProductionDataConfig.PAYLOAD_FILENAME
    )
    parser.add_argument(
        '--bm25_output_filename', type=str, default=YTProductionDataConfig.BM25_INDEX_FILENAME
    )
    parser.add_argument(
        '--dssm_knn_embeddings_output_filename', type=str, default=YTProductionDataConfig.DSSM_KNN_EMBEDDINGS_FILENAME
    )
    parser.add_argument(
        '--dssm_knn_model_output_filename', type=str, default=YTProductionDataConfig.DSSM_KNN_MODEL_FILENAME
    )
    args = parser.parse_args()

    if args.tempfolder is None:
        args.tempfolder = tempfile.mkdtemp()
        logger.debug('tempfolder was not specified, hence {} created'.format(args.tempfolder))

    fetch_texts_data(args.texts_table,
                     os.path.join(args.tempfolder, args.texts_output_filename),
                     os.path.join(args.tempfolder, args.payload_output_filename),
                     use_tqdm=True)
    fetch_bm25_data(args.bm25_token_id_mapping_table,
                    args.bm25_doc_lens_table,
                    os.path.join(args.tempfolder, args.bm25_output_filename),
                    use_tqdm=True)
    fetch_dssm_knn_data(args.dssm_knn_embeddings_table,
                        args.dssm_knn_model_file,
                        os.path.join(args.tempfolder, args.dssm_knn_embeddings_output_filename),
                        os.path.join(args.tempfolder, args.dssm_knn_model_output_filename),
                        use_tqdm=True)

    if os.path.exists(args.output_file):
        logger.debug('Removing previous output file {}'.format(args.output_file))
        os.remove(args.output_file)

    logger.debug('Starting making tar file {} from processed files'.format(args.output_file))
    with tarfile.open(args.output_file, 'w') as tar:
        for file in [
            os.path.join(args.tempfolder, args.texts_output_filename),
            os.path.join(args.tempfolder, args.payload_output_filename),
            os.path.join(args.tempfolder, args.bm25_output_filename),
            os.path.join(args.tempfolder, args.dssm_knn_embeddings_output_filename),
            os.path.join(args.tempfolder, args.dssm_knn_model_output_filename)
        ]:
            if os.path.exists(file):
                tar.add(file, arcname=os.path.basename(file))
