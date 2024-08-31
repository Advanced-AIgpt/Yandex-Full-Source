from .data_config import YTProductionDataConfig


class GenericConfig:
    # App defaults
    DEFAULT_N_SAMPLES = 10
    DEFAULT_APP_PORT = 5000
    DEFAULT_APP_NAME = 'DEFAULT APP'

    DEFAULT_SERVER_CONFIG = {
        'server': {
            'port': DEFAULT_APP_PORT,
            'verbose': True,
            'default_n_samples': DEFAULT_N_SAMPLES
        },
        'default_app': DEFAULT_APP_NAME,
        'apps': [
            {
                'name': DEFAULT_APP_NAME,
                'texts_path': YTProductionDataConfig.TEXTS_PATH,
                'payload_path': YTProductionDataConfig.PAYLOADS_PATH,
                'payload_keys': YTProductionDataConfig.PAYLOAD_KEYS,
                'default_index': 'DSSM with Logistic Regression + CatBoost',
                'indexes': [
                    {
                        'name': 'BM25',
                        'index_type': 'bm25',
                    },
                    {
                        'name': 'granet',
                        'index_type': 'granet',
                    },
                    {
                        'name': 'DSSM',
                        'index_type': 'dssm_knn'
                    },
                    {
                        'name': 'DSSM with Logistic Regression',
                        'index_type': 'logreg_dssm_knn',
                        'reuse_dssm_index_name': 'DSSM',
                    },
                    {
                        'name': 'DSSM with Logistic Regression with Active Learning',
                        'index_type': 'logreg_dssm_knn',
                        'active_learning': True,
                        'reuse_dssm_index_name': 'DSSM'
                    },
                    {
                        'name': 'DSSM with Logistic Regression + BM25',
                        'index_type': 'composite',
                        'reuse_indexes': ['DSSM with Logistic Regression', 'BM25']
                    },
                    {
                        'name': 'DSSM with Logistic Regression + CatBoost',
                        'index_type': 'logreg_with_catboost',
                        'reuse_weak_index_name': 'DSSM with Logistic Regression',
                        'reuse_dssm_index_name': 'DSSM'
                    }
                ]
            }
        ]
    }
