from .data_config import LocalProductionDataConfig


class MetricsConfig:
    ALL_INDEXES_CONFIGS = [
        {
            'index_type': 'bm25',
            'name': 'BM25'
        },
        {
            'index_type': 'dssm_knn',
            'name': 'DSSM'
        },
        {
            'index_type': 'logreg_dssm_knn',
            'name': 'LogReg'
        },
        {
            'index_type': 'logreg_dssm_knn',
            'active_learning': 'decision_boundary',
            'name': 'LogReg (decision_boundary)'
        },
        {
            'index_type': 'logreg_dssm_knn',
            'active_learning': 'multiple_boundaries',
            'name': 'LogReg (multiple_boundaries)'
        },
        {
            'index_type': 'logreg_dssm_knn',
            'active_learning': 'random_upon_bound',
            'name': 'LogReg (random_upon_bound)'
        },
        {
            'index_type': 'logreg_dssm_knn',
            'active_learning': 'random_upon_bound_by_probas',
            'name': 'LogReg (random_upon_bound_by_probas)'
        },
        {
            'index_type': 'logreg_with_catboost',
            'name': 'LogReg + CatBoost',
            'weak_index_config': {
                'index_type': 'logreg_dssm_knn'
            }
        },
        {
            'index_type': 'logreg_with_catboost',
            'name': 'LogReg (decision_boundary) + CatBoost',
            'weak_index_config': {
                'index_type': 'logreg_dssm_knn',
                'active_learning': 'decision_boundary'
            }
        },
        {
            'index_type': 'logreg_with_catboost',
            'name': 'LogReg (multiple_boundaries) + CatBoost',
            'weak_index_config': {
                'index_type': 'logreg_dssm_knn',
                'active_learning': 'multiple_boundaries'
            }
        },
        {
            'index_type': 'logreg_with_catboost',
            'name': 'LogReg (random_upon_bound) + CatBoost',
            'weak_index_config': {
                'index_type': 'logreg_dssm_knn',
                'active_learning': 'random_upon_bound'
            }
        },
        {
            'index_type': 'logreg_with_catboost',
            'name': 'LogReg (random_upon_bound_by_probas) + CatBoost',
            'weak_index_config': {
                'index_type': 'logreg_dssm_knn',
                'active_learning': 'random_upon_bound_by_probas'
            }
        },
    ]

    EXPERIMENT_CONFIG = {
        'report_config': {
            'path': './reports/'
        },

        'data_config': {
            'data_path': 'data/metrics/prepared/toloka_intents_to_texts.json',
            'min_samples': 2000,
            'query_len': 10,
            'train_data_size': 0.5
        },

        'indexes_defaults': {
            'bm25': {
                'index_path': None
            },

            'dssm_knn': {
                'model_path': LocalProductionDataConfig.DSSM_KNN_MODEL_PATH,
                'model_input_name': LocalProductionDataConfig.DSSM_KNN_MODEL_INPUT_NAME,
                'model_output_name': LocalProductionDataConfig.DSSM_KNN_MODEL_OUTPUT_NAME,
                'embeddings_path': None
            },

            'logreg_dssm_knn': {
                'dssm_index_config': {
                    'index_type': 'dssm_knn',
                    'model_path': LocalProductionDataConfig.DSSM_KNN_MODEL_PATH,
                    'model_input_name': LocalProductionDataConfig.DSSM_KNN_MODEL_INPUT_NAME,
                    'model_output_name': LocalProductionDataConfig.DSSM_KNN_MODEL_OUTPUT_NAME,
                    'embeddings_path': None
                },
                'logreg_min_negative_samples': 50,
                'C': 100.0,
                'active_learning': None
            },

            'logreg_with_catboost': {
                'weak_index_config': {
                    'index_type': 'logreg_dssm_knn',
                    'dssm_index_config': {
                        'index_type': 'dssm_knn',
                        'model_path': LocalProductionDataConfig.DSSM_KNN_MODEL_PATH,
                        'model_input_name': LocalProductionDataConfig.DSSM_KNN_MODEL_INPUT_NAME,
                        'model_output_name': LocalProductionDataConfig.DSSM_KNN_MODEL_OUTPUT_NAME,
                        'embeddings_path': None
                    },
                    'logreg_min_negative_samples': 50,
                    'C': 100.0,
                    'active_learning': None
                },
                'dssm_index_config': {
                    'index_type': 'dssm_knn',
                    'model_path': LocalProductionDataConfig.DSSM_KNN_MODEL_PATH,
                    'model_input_name': LocalProductionDataConfig.DSSM_KNN_MODEL_INPUT_NAME,
                    'model_output_name': LocalProductionDataConfig.DSSM_KNN_MODEL_OUTPUT_NAME,
                    'embeddings_path': None
                }
            }
        },

        'indexes_configs': [(train_index, val_index) for train_index in ALL_INDEXES_CONFIGS
                            for val_index in ALL_INDEXES_CONFIGS],

        'rounds_config': {
            'n_samples_each_round': 50,
            'n_rounds': 20
        },

        'metrics_config': [
            {
                'name': 'MPD@',
                'metric_type': 'mean_pairwise_distance_at_k',
                'model_path': LocalProductionDataConfig.DSSM_KNN_MODEL_PATH,
                'model_input_name': LocalProductionDataConfig.DSSM_KNN_MODEL_INPUT_NAME,
                'model_output_name': LocalProductionDataConfig.DSSM_KNN_MODEL_OUTPUT_NAME,
                'text_preprocessing_fn': 'alice_dssm_applier_preprocessing',
                'k_values': [50]
            },
            {
                'name': 'Precision@',
                'metric_type': 'precision_at_k',
                'k_values': [10, 50]
            },
            {
                'name': 'ROC AUC',
                'metric_type': 'roc_auc'
            }
        ]
    }
