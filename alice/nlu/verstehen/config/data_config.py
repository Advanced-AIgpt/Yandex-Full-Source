import os


class MetricsDataConfig:
    ROOT = os.path.join('data/metrics')
    SOURCE = os.path.join(ROOT, 'source')
    PREPARED = os.path.join(ROOT, 'prepared')

    # intents and their NLUs data without toloka assisted data (extracted from VINS) (source for queries data)
    NLU_INTENT_TO_TEXTS_PATH = os.path.join(SOURCE, 'nlu_intents_to_texts.json')

    # intents and their NLUs data with toloka assisted data (extracted from VINS) (source for queries data)
    NLU_INTENT_TEXTS_WITH_TOLOKA_PATH = os.path.join(SOURCE, 'nlu_intents_to_texts_with_toloka.json')

    # file containing mapping from toloka daily markup and VINS intents (copied from VINS)
    TOLOKA_INTENT_RENAMES_PATH = os.path.join(SOURCE, 'toloka_intent_renames.json')

    # toloka daily markup in TSV format (exported from YT)
    TOLOKA_TSV_PATH = os.path.join(SOURCE, 'toloka_daily_yt.tsv')

    # toloka daily markup processed from TOLOKA_TSV_PATH (source for index data)
    TOLOKA_INTENT_TO_TEXTS_PATH = os.path.join(PREPARED, 'toloka_intents_to_texts.json')

    # prepared data for metrics computations
    PREPARED_INDEX_DATA_PATH = os.path.join(PREPARED, 'index_intents_to_texts.json')
    PREPARED_QUERIES_DATA_PATH = os.path.join(PREPARED, 'queries_intents_to_texts.json')


class YTProductionDataConfig:
    # paths to tables and files on hahn
    YT_DEFAULT_PREFIX = '//home/voice/artemkorenev/verstehen/quasar'

    YT_TEXTS_TABLE = os.path.join(YT_DEFAULT_PREFIX, 'texts/texts_table')
    YT_BM25_TOKEN_ID_MAPPING_TABLE = os.path.join(YT_DEFAULT_PREFIX, 'bm25/token_id_mapping')
    YT_BM25_DOC_LENS_TABLE = os.path.join(YT_DEFAULT_PREFIX, 'bm25/doc_lens')

    YT_DSSM_KNN_EMBEDDINGS_TABLE = os.path.join(YT_DEFAULT_PREFIX, 'dssm_knn/embeddings')
    YT_DSSM_KNN_MODEL_FILE = os.path.join(YT_DEFAULT_PREFIX, 'dssm_knn/model')

    # output paths
    ROOT = os.path.join('all_bucket')

    TEXTS_FILENAME = 'texts.pickle'
    PAYLOAD_FILENAME = 'payload.pickle'
    PAYLOAD_KEYS = ['request_id', 'uuid', 'app_id', 'occurrence_rate']
    BM25_INDEX_FILENAME = 'bm25_index.pickle'
    DSSM_KNN_EMBEDDINGS_FILENAME = 'dssm_knn_embeddings.bytes'
    DSSM_KNN_MODEL_FILENAME = 'dssm_knn.model'

    TEXTS_PATH = os.path.join(ROOT, TEXTS_FILENAME)
    PAYLOADS_PATH = os.path.join(ROOT, PAYLOAD_FILENAME)
    BM25_INDEX_PATH = os.path.join(ROOT, BM25_INDEX_FILENAME)
    DSSM_KNN_EMBEDDINGS_PATH = os.path.join(ROOT, DSSM_KNN_EMBEDDINGS_FILENAME)
    DSSM_KNN_EMBEDDINGS_DIM = 300

    DSSM_KNN_MODEL_PATH = os.path.join(ROOT, DSSM_KNN_MODEL_FILENAME)
    DSSM_KNN_MODEL_INPUT_NAME = 'context_0'
    DSSM_KNN_MODEL_OUTPUT_NAME = 'query_embedding'


class LocalProductionDataConfig:
    ROOT = os.path.join('data/local/prod')
    SOURCE = os.path.join(ROOT, 'source')
    PREPARED = os.path.join(ROOT, 'prepared')

    # big index data without intents in TSV format (exported from YT)
    BIG_TSV_SOURCE_PATH = os.path.join(
        SOURCE, 'yt_tmp_artemkorenev_a716a39a-382e8feb-8dcf1ce5-e'
    )

    DSSM_KNN_MODEL_PATH = os.path.join('data', 'dssm', 'dssm_model.apply')
    DSSM_KNN_MODEL_INPUT_NAME = 'context_0'
    DSSM_KNN_MODEL_OUTPUT_NAME = 'query_embedding'

    # prepared data for index from BIG_TSV_SOURCE_PATH
    BIG_PREPARED_DATA_PATH = os.path.join(PREPARED, 'big_index_data.json')
