# coding: utf-8
from __future__ import unicode_literals

from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME


def test_new_conf(app_config):
    assert app_config.nlu == {
        'feature_extractors': [
            {
                'id': 'word',
                'type': 'ngrams',
                'n': 1
            }
        ],
        'fallback_threshold': 0.6,
        'intent_classifiers': [{
            'model': 'maxent',
            'name': 'test_clf',
            'features': ['word']
        }],
        'fallback_intent_classifiers': [{
            'model': 'maxent',
            'name': 'test_fallback_clf',
            'features': ['word']
        }],
        'utterance_tagger': {
            'model': 'crf',
            'features': ['word'],
            'params': {'intent_conditioned': True}
        },
        'samples_extractor': {
            'pipeline': [
                {
                    'max_tokens': 256,
                    'name': 'clip'
                },
                {
                    'name': 'normalizer',
                    'normalizer': DEFAULT_RU_NORMALIZER_NAME
                }
            ]
        },
        'fst': {
            'resource': 'resource://fst',
            'parsers': [
                'units_time',
                'datetime',
                'date',
                'time',
                'geo',
                'num',
                'fio',
                'datetime_range',
                'poi_category_ru',
                'currency',
                'float',
                'calc',
                'weekdays',
                'soft',
                'site',
                'album',
                'artist',
                'track',
                'films_100_750',
                'films_50_filtered',
                'swear'
            ]
        }
    }

    for i in app_config.intents:
        assert i.name
        assert i.dm
        assert i.nlg or i.nlu

    assert sorted([i.name for i in app_config.intents]) == [
        'test_bot._internal_.dont_understand',
        'test_bot.general.intent1',
        'test_bot.general.intent2',
        'test_bot.general.intent3',
        'test_bot.general.intent4',
        'test_bot.general.micro1',
    ]
    intents_map = {
        i.name: i
        for i in app_config.intents
    }
    assert intents_map['test_bot.general.intent1'].nlu.config == [{'source': 'file', 'path': 'vins_core/test/test_data/test_app/general/intents/intent1.nlu'}]
    assert intents_map['test_bot.general.intent2'].nlu.config == [{'source': 'file', 'path': 'vins_core/test/test_data/test_app/general/intents/intent2.nlu'}]

    assert intents_map['test_bot.general.intent1'].nlg_filename

    assert intents_map['test_bot.general.intent1'].dm
    assert intents_map['test_bot.general.intent1'].dm.name == 'test_bot.general.intent1'


def test_additional_intent_properties(app_with_intents_for_metric):
    app = app_with_intents_for_metric
    # test the literal config
    assert app.nlu.intent_infos[
        'general.the_funniest_intent_ever'
    ].negative_sampling_from == 'the_most_boring_intent_ever|the_ugliest_intent_ever'
    assert app.nlu.intent_infos[
        'general.the_ugliest_intent_ever'
    ].negative_sampling_from == 'the_funniest_intent_ever'
    assert app.nlu.intent_infos['general.the_funniest_intent_ever'].positive_sampling is True
    assert app.nlu.intent_infos['general.the_most_boring_intent_ever'].positive_sampling is False
    # test the default values
    assert app.nlu.intent_infos['general.the_ugliest_intent_ever'].positive_sampling is True
    assert app.nlu.intent_infos['general.the_most_boring_intent_ever'].negative_sampling_from is None
