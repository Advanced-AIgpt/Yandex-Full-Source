# flake8: noqa
from __future__ import unicode_literals

from alice.vins.api_helper.standard_settings import *  # noqa
from alice.vins.api_helper.standard_settings import LOGGING

from vins_core.utils.config import get_setting

CONNECTED_APPS = {
    'pa': {
        'path': 'personal_assistant/config/Vinsfile.json',
        'class': 'personal_assistant.app.PersonalAssistantApp',
        'ignore_mongo_errors': True,
        'resource': 'sk',
        'intent_renames': 'personal_assistant/tests/validation_sets/toloka_intent_renames.json'
    },
    'navi': {
        'path': 'navi_app/config/Vinsfile.json',
        'class': 'navi_app.app.NaviApp',
        'resource': 'navi',
        'ignore_mongo_errors': True,
    },
}


slow_response_bins = range(100, 10000, 500) + [500000]
delayed_response_bins = [1, 10, 30, 50] + range(100, 5000, 300) + [10000]
quick_response_bins = [1, 3, 5, 8, 10, 20, 40, 80, 150, 300, 500, 700, 1000, 2000, 5000]
fast_response_bins = [1, 2, 3, 5, 8, 12, 16, 20, 25, 40, 80, 100, 150, 200, 300, 500, 1000, 5000]

VINS_METRICS_CONF = {
    # types igauge, dgauge, counter, rate, timer
    'app_handle_request_time': {'type': 'hist', 'bins': delayed_response_bins},
    'app_load_session_time': {'type': 'hist', 'bins': quick_response_bins},
    'app_save_session_time': {'type': 'hist', 'bins': quick_response_bins},

    'bass_response': {'type': 'rate'},
    'bass_response_time': {'bins': delayed_response_bins, 'type': 'hist'},
    'bass_setup_time': {'bins': delayed_response_bins, 'type': 'hist'},

    'ce_updater_download_arch_time': {'bins': slow_response_bins, 'type': 'hist'},
    'ce_updater_get_last_build_time': {'bins': slow_response_bins, 'type': 'hist'},

    'dm_handle_form_time': {'type': 'hist', 'bins': delayed_response_bins},
    'dm_handle_time': {'type': 'hist', 'bins': delayed_response_bins},
    'dm_samples_extractor_time': {'type': 'hist', 'bins': quick_response_bins},
    'dm_update_custom_entities_time': {'bins': quick_response_bins, 'type': 'hist'},
    'dm_postprocessing_time': {'bins': fast_response_bins, 'type': 'hist'},

    'entitysearch_response_time': {'bins': fast_response_bins, 'type': 'hist'},
    'entitysearch_response': {'type': 'rate'},

    'gc_response': {'type': 'rate'},
    'gc_response_time': {'bins': fast_response_bins, 'type': 'hist'},

    'http_requests': {'type': 'rate'},
    'http_response_exceptions': {'type': 'rate'},
    'http_response_time': {'type': 'hist', 'bins': delayed_response_bins},
    'http_responses': {'type': 'rate'},

    'knn_predict_time': {'bins': quick_response_bins, 'type': 'hist'},
    'knn_exact_match_cache': {'type': 'rate'},
    'knn_redis_cache': {'type': 'rate'},

    'nlu_extract_entities_time': {'bins': fast_response_bins, 'type': 'hist'},
    'nlu_extract_features_time': {'bins': fast_response_bins, 'type': 'hist'},
    'nlu_fst_parser_parse_time': {'bins': fast_response_bins, 'type': 'hist'},
    'nlu_handle_time': {'bins': quick_response_bins, 'type': 'hist'},
    'nlu_predict_fallback_intent_time': {'bins': quick_response_bins, 'type': 'hist'},
    'nlu_predict_intents_time': {'bins': quick_response_bins, 'type': 'hist'},
    'nlu_predict_tags_time': {'bins': fast_response_bins, 'type': 'hist'},

    'misspell_response_time': {'bins': fast_response_bins, 'type': 'hist'},
    'misspell_response': {'type': 'rate'},

    'mongo_load_session': {'type': 'rate'},
    'mongo_load_session_time': {'type': 'hist', 'bins': fast_response_bins},
    'mongo_save_session': {'type': 'rate'},
    'mongo_save_session_time': {'type': 'hist', 'bins': fast_response_bins},
    'mongo_error': {'type': 'rate'},

    'pa_bass_error_block': {'type': 'rate'},
    'pa_callback_time': {'type': 'hist', 'bins': delayed_response_bins},
    'pa_handle_response_time': {'bins': fast_response_bins, 'type': 'hist'},
    'pa_hit_scenario': {'type': 'rate'},
    'pa_update_entity': {'type': 'rate'},

    's3_response': {'type': 'rate'},
    's3_response_time': {'bins': quick_response_bins, 'type': 'hist'},

    'serp_features_response': {'type': 'rate'},
    'serp_features_response_time': {'bins': fast_response_bins, 'type': 'hist'},

    'source_http_request': {'type': 'rate'},
    'source_http_request_time': {'type': 'hist', 'bins': slow_response_bins},

    'updater_has_update': {'type': 'rate'},
    'updater_get_update_time': {'type': 'hist', 'bins': slow_response_bins},

    'view_parse_time': {'bins': fast_response_bins, 'type': 'hist'},
    'view_handle_request_time': {'type': 'hist', 'bins': delayed_response_bins},
    'view_serialize_time': {'bins': fast_response_bins, 'type': 'hist'},

    'wizard_response': {'type': 'rate'},
    'wizard_response_time': {'bins': fast_response_bins, 'type': 'hist'},

    'session_size': {'type': 'hist', 'bins': [500, 1000, 3000, 5000, 10000, 20000, 30000, 50000, 10**6]},
    'sessions_sent': {'type': 'rate'},
    'sessions_received': {'type': 'rate'},

    'qa_api_features_response_time': {'type': 'hist', 'bins': delayed_response_bins},
    'qa_api_features_vins_response_time': {'type': 'hist', 'bins': delayed_response_bins},
    'qa_api_features_protobuf_serialize': {'bins': quick_response_bins, 'type': 'hist'},

    'rtlog_active_loggers': {'type': 'igauge'},
    'rtlog_events': {'type': 'rate'},
    'rtlog_pending_bytes': {'type': 'igauge'},
    'rtlog_written_frames': {'type': 'rate'},
    'rtlog_written_bytes': {'type': 'rate'},
    'rtlog_errors': {'type': 'rate'},
    'rtlog_shrinked_bytes': {'type': 'rate'},
}


if get_setting('METRICS_FILE', default=''):
    LOGGING['handlers']['metrics_handler'] = {
        'class': 'vins_core.utils.logging.AsyncWatchedFileHandler',
        'filename': get_setting('METRICS_FILE'),
        'queue_size': 100000,
        'level': 'DEBUG',
        'formatter': 'message_only'
    }
    LOGGING['loggers']['metrics_logger'] = {
        'handlers': ['metrics_handler'],
        'level': 'DEBUG',
        'propagate': True,
    }
