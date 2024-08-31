# coding: utf-8
from __future__ import unicode_literals

import logging
import mock
import os
import pytest

from copy import deepcopy
from uuid import uuid4

import vins_core
from vins_core.common.sample import Sample
from vins_core.config.app_config import AppConfig
from vins_core.ner.fst_base import Entity
from vins_core.ner.fst_presets import FstParserFactory
from vins_core.nlu.base_nlu import BaseNLU, IntentCandidate
from vins_core.dm.intent import Intent
from vins_core.dm.form_filler.models import Form, Handler
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.form_filler.nlu_postprocessor import (
    SortNluPostProcessor, IntentGroupNluPostProcessor,
    ChainedNluPostProcessor, FirstFrameChoosingNluPostProcessor
)
from vins_core.dm.response import VinsResponse

from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.flow_nlu import _DEFAULT_FALLBACK_THRESHOLD
from vins_core.dm.request import create_request, AppInfo
from vins_core.dm.session import Session
from vins_core.dm.formats import NluSourceItem
from vins_core.config.app_config import Project
from vins_sdk.app import VinsApp
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.nlu.base_nlu import NLUHandleResult
from vins_core.nlu.features.cache.picklecache import SampleFeaturesCache


frame_location_to_geo = [{
    'intent_candidate': IntentCandidate(name='taxi', score=1.0),
    'intent_name': 'taxi',
    'confidence': 1.0,
    'entities': [],
    'tagger_score': 0.8,
    'slots': {
        'location_to': [
            {
                'start': 3,
                'end': 4,
                'substr': 'домодедово',
                'entities': [
                    Entity(start=3, end=4, type='GEO', value={'city': 'г. Домодедово'})
                ]
            }
        ]
    }
}]

frame_location_to_not_geo = [{
    'intent_candidate': IntentCandidate(name='taxi', score=1.0),
    'intent_name': 'taxi',
    'confidence': 1.0,
    'entities': [],
    'tagger_score': 0.8,
    'slots': {
        'location_to': [
            {
                'start': 3,
                'end': 4,
                'substr': 'домодедово',
                'entities': []
            }
        ]
    }
}]


def build_dm_config(location_to_types=None):
    intents = [
        Intent('taxi'),
        Intent('alarm')
    ]
    forms_map = {
        'taxi': Form.from_dict({
            'name': 'taxi',
            'slots': [
                {
                    'slot': 'location_from',
                    'types': ['geo'],
                    'optional': False
                }, {
                    'slot': 'location_to',
                    'types': location_to_types or [],
                    'optional': False
                }, {
                    'slot': 'when',
                    'types': ['datetime'],
                    'optional': False
                }
            ]
        }),
        'alarm': Form.from_dict({
            'name': 'alarm',
            'slots': [
                {
                    'slot': 'when',
                    'types': ['datetime'],
                    'optional': False
                }
            ]
        })
    }
    return intents, forms_map


def combine_classifiers(classifiers_list, name):
    return {
        "model": "combine_scores",
        "name": name,
        "params": {
            "classifiers": classifiers_list,
            "method": "multiply"
        }
    }


@pytest.fixture
def session():
    return Session(app_id='123', uuid=uuid4())


@pytest.mark.parametrize("specified_types,nlu_frames,output_slot_value", [
    # geo object is presented
    (['geo'], frame_location_to_geo, {'city': 'г. Домодедово'}),
    (['geo.city'], frame_location_to_geo, 'г. Домодедово'),
    (['geo', 'string'], frame_location_to_geo, {'city': 'г. Домодедово'}),
    (['geo.city', 'string'], frame_location_to_geo, 'г. Домодедово'),
    (['string'], frame_location_to_geo, 'домодедово'),

    # geo object is not presented
    (['geo'], frame_location_to_not_geo, None),
    (['geo.city'], frame_location_to_not_geo, None),
    (['geo', 'string'], frame_location_to_not_geo, 'домодедово'),
    (['geo.city', 'string'], frame_location_to_not_geo, 'домодедово'),
    (['string'], frame_location_to_not_geo, 'домодедово'),
])
def test_location_entity_resolved(specified_types, nlu_frames, output_slot_value, session, mocker):

    intents, forms_map = build_dm_config(location_to_types=specified_types)

    def base_nlu_handle(*args, **kwargs):
        return NLUHandleResult(semantic_frames=nlu_frames, sample_features=None)

    dm = DialogManager(
        intents,
        forms_map,
        nlu=BaseNLU(
            fst_parser_factory=FstParserFactory(None, {}),
            fst_parsers=[],
            intent_classifiers=None,
            utterance_tagger=None
        ),
        nlg=None
    )
    mocker.patch.object(BaseNLU, "handle", base_nlu_handle)
    mocker.patch.object(dm, "_ask_slot")
    mocker.patch.object(dm, "_submit_form")
    dm.handle(create_request(uuid=session.uuid, utterance='dummy'), session=session, app=object(), response=None)
    assert session.form.get_slot_by_name('location_to').value == output_slot_value


def test_sample_features_in_callback(mocker, dm, session):
    def mock_handle(*args, **kwargs):
        assert 'sample_features' in kwargs
        sf = kwargs['sample_features']
        assert isinstance(sf, SampleFeatures)

    mocker.patch.object(Handler, "handle", mock_handle)
    dm.handle(create_request(uuid=session.uuid, utterance='dummy'), session=session, app=object(), response=None)


def test_nlu_result_sample_features_consistent(simple_app, session):
    app = simple_app.vins_app
    dm = app.dm
    req_info = create_request(uuid=session.uuid, utterance='какого цвета яблоко')
    sample = dm.samples_extractor(
        [req_info.utterance], session, filter_errors=True, app_id=req_info.app_info.app_id or ''
    )[0]
    nlu_result_dm = dm.handle(
        req_info,
        session=session,
        app=app,
        response=VinsResponse(),
    )
    nlu_result_nlu = dm.nlu.handle(
        sample,
        session,
        response=VinsResponse(),
        req_info=req_info,
    )
    dm_sample_features = nlu_result_dm.sample_features
    nlu_sample_features = nlu_result_nlu.sample_features
    dm_sample_features.clear_scores()
    nlu_sample_features.clear_scores()
    assert dm_sample_features.to_pb() == nlu_sample_features.to_pb()


def get_intent(config, text, session, req_info):
    dm = DialogManager.from_config(config, load_data=True)
    dm.nlu.train()
    result = dm.nlu.handle(Sample.from_string(text), session, req_info=req_info)
    return result.semantic_frames[0]['intent_name']


@pytest.mark.parametrize("classifier_source, recognized_intent", [
    ({'test_bot._internal_.dont_understand': ['текст 4']}, 'test_bot._internal_.dont_understand'),
    ({'test_bot._internal_.dont_understand': ['текст 5']}, 'test_bot.general.intent1'),
    (None, 'test_bot.general.intent1')
])
def test_simple_aux_classifier(classifier_source, recognized_intent, app_config, session, req_info):
    if classifier_source:
        app_config.nlu['intent_classifiers'].insert(0, {
            'model': 'data_lookup',
            'name': 'the_aux_classifier',
            'params': {
                'intent_texts': classifier_source
            }
        })
    assert get_intent(app_config, 'текст 4', session, req_info) == recognized_intent


@pytest.mark.parametrize("with_update", (True, False))
def test_simple_aux_classifier_abnormal_intent(app_config, session, req_info, with_update):
    app_config.nlu['intent_classifiers'].insert(0, {
        'model': 'data_lookup',
        'params': {
            'intent_texts': {}
        },
        'name': 'ext_clf'
    })
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    with mock.patch.object(
        vins_core.nlu.lookup_classifier.DataLookupTokenClassifier, '_iter_lookup_data',
        return_value=[('abnormal_intent', 'текст 4')]
    ):
        if with_update:
            dm.nlu.get_classifier('ext_clf')._update()
        result = dm.nlu.handle(Sample.from_string('текст 4'), session, req_info=req_info).semantic_frames
        assert result[0]['intent_name'] == 'test_bot.general.intent1'


def register_intent(name, nlg_phrase, nlu=None, trainable_classifiers=None):
    intent = {
        "intent": name,
        "dm": {
            "data": {
                "form": name,
                "events": [{
                    "event": "submit",
                    "handlers": [{
                        "handler": "say",
                        "say": nlg_phrase
                    }]
                }]
            }
        }
    }
    if nlu:
        intent['nlu'] = [{
            "source": "data",
            "data": nlu
        }]
    if trainable_classifiers is not None:
        intent['trainable_classifiers'] = trainable_classifiers
    return intent


@pytest.mark.slowtest
def test_external_skill_classifier(app_config, session):
    # 1. external skills activation and deactivation (could be also a part of general app intents)
    app_config.nlu['intent_classifiers'].insert(0, {
        'model': 'data_lookup',
        'name': 'external_skills',
        'params': {
            'intent_texts': {
                'test_bot.commands.external_skill_1': ['Включай первый скил'],
                'test_bot.commands.external_skill_2': ['Включай второй скил'],
                'test_bot.commands.external_skills_deactivation': ['Алиса, вернись!'],
            }
        }
    })

    # 2. proxy state for external skills
    app_config.nlu['intent_classifiers'].insert(1, {
        'model': 'data_lookup',
        'name': 'proxy',
        'params': {
            'intent_texts': {'test_bot.external_skills.proxy': ['.*']},
            'regexp': True
        }
    })

    # Register associated new intents & forms
    app_config.projects.append(Project.from_dict({
        "name": "commands",
        "intents": [
            register_intent("external_skill_1", "Включаю первый скил"),
            register_intent("external_skill_2", "Включаю второй скил"),
            register_intent("external_skills_deactivation", "Я вернулась!")
        ]
    }, base_name='test_bot'))

    app_config.projects.append(Project.from_dict({
        "name": "external_skills",
        "intents": [
            register_intent("proxy", "Работает внешний скил...")
        ]
    }, base_name='test_bot'))

    # Register transition model, that allows:
    app_config.nlu['transition_model'] = {
        'model_name': 'from_data',
        'data': {
            # starting from app intents,
            (None, 'test_bot.general.intent1'): 1,
            ('test_bot.general.intent1', 'test_bot.general.intent1'): 1,
            # transfering from app intents to external skills via external skill activation,
            ('test_bot.general.intent1', 'test_bot.commands.external_skill_1'): 1,
            ('test_bot.general.intent1', 'test_bot.commands.external_skill_2'): 1,
            # staying infinitely in external skill...
            ('test_bot.commands.external_skill_1', 'test_bot.external_skills.proxy'): 1,
            ('test_bot.external_skills.proxy', 'test_bot.external_skills.proxy'): 1,
            # ...until deactivation is invoked,
            ('test_bot.external_skills.proxy', 'test_bot.commands.external_skills_deactivation'): 1,
            # than going back to app intents...
            ('test_bot.commands.external_skills_deactivation', 'test_bot.general.intent1'): 1,
            ('test_bot.commands.external_skills_deactivation', 'test_bot.general.intent2'): 1,
            # ...or another skill
            ('test_bot.commands.external_skills_deactivation', 'test_bot.commands.external_skill_1'): 1,
            ('test_bot.commands.external_skills_deactivation', 'test_bot.commands.external_skill_2'): 1
        }
    }
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    app = VinsApp(dm)

    def f(text):
        return app.handle_request(create_request(uuid=session.uuid, utterance=text)).voice_text

    assert f('текст 1') == 'фраза 1'
    assert f('включай первый скил') == 'Включаю первый скил'
    assert f('фраза 1') == 'Работает внешний скил...'
    assert f('не, больше не хочу') == 'Работает внешний скил...'
    assert f('Включай второй скил') == 'Работает внешний скил...'
    assert f('Алиса вернись') == 'Я вернулась!'
    assert f('Включай второй скил') == 'Включаю второй скил'


def test_cascade_fallback(app_config, session, req_info):
    app_config.nlu['intent_classifiers'].insert(0, {
        'model': 'data_lookup',
        'name': 'fixlist',
        'params': {
            'intent_texts': {
                'test_bot.general.micro1': ['1']
            }
        }
    })
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    sample = Sample.from_string('1')
    result = dm.nlu.handle(sample, session, req_info=req_info).semantic_frames
    assert result[0]['intent_name'] == 'test_bot.general.micro1'
    intent_scores = dm.nlu.get_classifier('test_clf')(dm.nlu.features_extractor([sample])[0])
    assert max(intent_scores.values()) < _DEFAULT_FALLBACK_THRESHOLD


@pytest.fixture(scope='function')
def app_config_with_cascade(app_config):
    app_config.nlu['intent_classifiers'][0]['name'] = 'top'
    app_config.nlu['intent_classifiers'].insert(0, {
        'model': 'data_lookup',
        'name': 'fixlist',
        'params': {
            'intent_texts': {
                'test_bot.general.micro1': ['1']
            }
        }
    })
    app_config.nlu['feature_extractors'] = [
        {'type': 'ngrams', 'id': 'word', 'n': 1}
    ]
    app_config.nlu['intent_classifiers'].append({
        'model': 'maxent',
        'name': 'bottom',
        'features': ['word']
    })
    for intent in app_config.intents:
        if intent.name == 'test_bot.general.intent1':
            intent.trainable_classifiers = ['top']
        elif intent.name == 'test_bot.general.intent2':
            intent.trainable_classifiers = ['top', 'bottom']
        elif intent.name == 'test_bot.general.micro1':
            intent.trainable_classifiers = ['bottom']
    return app_config


@pytest.mark.parametrize("input, output", [
    ('1', 'test_bot.general.micro1'),
    ('2', 'test_bot.general.intent2')
])
def test_cascade_intent_classifier(app_config_with_cascade, session, req_info, input, output):
    assert get_intent(app_config_with_cascade, input, session, req_info) == output


@pytest.mark.parametrize('thresholds, input, intent_result', [
    ([0.5, 0.5, 0.5], 'текст фраза', 'test_bot.general.intent1'),
    ([0.5, 0.6, 0.5], 'текст фраза', 'test_bot.general.intent2'),
    (None, 'текст фраза', 'test_bot.general.intent2'),
    ([0.5, 0.9, 0.99], 'текст фраза', 'test_bot._internal_.dont_understand'),
])
def test_cascade_thresholds(app_config_with_cascade, session, req_info, thresholds, input, intent_result):
    if thresholds is not None:
        for i in xrange(len(thresholds)):
            app_config_with_cascade.nlu['intent_classifiers'][i]['fallback_threshold'] = thresholds[i]
    assert get_intent(app_config_with_cascade, input, session, req_info) == intent_result


@pytest.mark.parametrize('input, trigger_fallback, intent_result', [
    ('test nlu record', False, 'test_bot.general.trigger'),
    ('test nlu record', True, 'test_bot.general.micro1')
])
def test_trigger_intent(app_config_with_cascade, session, req_info, input, trigger_fallback, intent_result):
    for intent in app_config_with_cascade.intents:
        if intent.name == 'test_bot.general.intent1':
            intent.trainable_classifiers = ['top']
        elif intent.name == 'test_bot.general.intent2':
            intent.trainable_classifiers = ['bottom']
        elif intent.name == 'test_bot.general.micro1':
            intent.trainable_classifiers = ['bottom']

    app_config_with_cascade.projects.append(Project.from_dict({
        'name': 'general',
        'intents': [{
            'intent': 'trigger',
            'trainable_classifiers': ['top'],
            'fallback': trigger_fallback,
            'nlu': [
                {'source': 'file', 'path': 'vins_core/test/test_data/test_app/general/intents/intent2.nlu'},
                {'source': 'microintents', 'path': 'vins_core/test/test_data/test_app/general/intents/micro.microintents.yaml'}
            ],
            'dm': {'data': {'name': 'trigger'}}
        }]
    }, base_name='test_bot'))
    assert get_intent(app_config_with_cascade, input, session, req_info) == intent_result


@pytest.fixture(params=[True, False])
def additional_nlu_source_tsv_file(request, tmpdir):
    tsv_file = tmpdir.join('additional_nlu_source.tsv')
    tsv_file.write_text(
        'text\tintent\n'
        'новая фраза "ххх"(new1) для первого интента\tintent1\n'
        'новая фраза "yyy"(new2) для второго интента\tintent2\n', 'utf-8')
    tsv_source = {
        'source': 'tsv',
        'path': tsv_file.strpath,
        'text_column': 'text',
        'row_filters': {},
        'header': 0,
        'can_use_to_train_tagger': request.param
    }
    yield tsv_source
    tmpdir.remove()


def _additional_nlu_sources_configure_and_run(app_config, source_config, input, session, req_info):
    for intent_cfg in app_config.intents:
        if 'intent' in intent_cfg.name:
            cfg = deepcopy(source_config)
            cfg['row_filters']['intent'] = intent_cfg.name.split('.')[-1]
            intent_cfg._nlu_sources.config.append(cfg)

    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    result = dm.nlu.handle(Sample.from_string(input), session, req_info=req_info).semantic_frames[0]
    return result['intent_name'], result['slots']


def _test_additional_nlu_sources(
    app_config, session, additional_nlu_sources, input, intent_found, slot_found, req_info
):
    intent, slots = _additional_nlu_sources_configure_and_run(
        app_config, additional_nlu_sources, input, session, req_info)
    assert intent == intent_found
    assert 'can_use_to_train_tagger' in additional_nlu_sources
    if additional_nlu_sources['can_use_to_train_tagger']:
        assert (slot_found in slots) or (not slots and not slot_found)
    else:
        assert not slots


@pytest.mark.parametrize('input, intent_found, slot_found', [
    ('новая фраза ххх для первого интента', 'test_bot.general.intent1', 'new1'),
    ('новая фраза ххх', 'test_bot._internal_.dont_understand', None)
])
def test_additional_nlu_sources_tsv_file(
    app_config, session, req_info, additional_nlu_source_tsv_file, input, intent_found, slot_found
):
    _test_additional_nlu_sources(app_config, session, additional_nlu_source_tsv_file,
                                 input, intent_found, slot_found, req_info)


@pytest.mark.parametrize("banlist, input, intent_result", [
    ({'test_bot.general.intent1': ['текст.*']}, 'текст 1', 'test_bot.general.intent2'),
    ({'test_bot.general.intent1': ['фраза.*']}, 'текст 1', 'test_bot.general.intent1'),
    ({'test_bot.general.intent1': ['фраза.*'], 'test_bot.general.intent2': ['фраза.*']},
     'текст 1', 'test_bot.general.intent1'),
    ({'test_bot.general.intent1': ['текст.*'], 'test_bot.general.intent2': ['текст.*']},
     'текст 1', 'test_bot.general.micro1'),
    ({'test_bot.general.intent1': ['текст.*'], 'test_bot.general.intent2': ['текст.*'],
      'test_bot.general.micro1': ['текст.*']},
     'текст 1', 'test_bot._internal_.dont_understand'),
    ({'test_bot.general.intent1': ['текст.*'], 'test_bot.general.intent2': ['текст.*'],
      'test_bot.general.micro1': ['test.*']},
     'текст 1', 'test_bot.general.micro1'),
    ({'test_bot.general.intent1': ['текст.*'], 'test_bot.general.intent2': ['текст.*'],
      'test_bot.general.micro1': ['текст.*']},
     'фраза', 'test_bot.general.intent2'),
])
def test_banlist_within_cascade(app_config_with_cascade, banlist, input, intent_result, session, req_info):
    banlist_obj = {
        'model': 'data_lookup',
        'name': 'banlist',
        'params': {
            'intent_texts': banlist,
            'matching_score': 0,
            'default_score': 1,
            'regexp': True
        }
    }
    for intent in app_config_with_cascade.intents:
        if intent.name == 'test_bot.general.intent1':
            intent.trainable_classifiers = ['top']
        elif intent.name == 'test_bot.general.intent2':
            intent.trainable_classifiers = ['top']
        elif intent.name == 'test_bot.general.micro1':
            intent.trainable_classifiers = ['bottom']

    app_config_with_cascade.nlu['intent_classifiers'][1] = combine_classifiers([
        banlist_obj, app_config_with_cascade.nlu['intent_classifiers'][1]
    ], name='combine_classifiers_1')
    app_config_with_cascade.nlu['intent_classifiers'][2] = combine_classifiers([
        banlist_obj, app_config_with_cascade.nlu['intent_classifiers'][2]
    ], name='combine_classifiers_2')
    assert get_intent(app_config_with_cascade, input, session, req_info) == intent_result


@pytest.fixture(scope='module')
def dm(app_config_session_scope):
    dm = DialogManager.from_config(app_config_session_scope, load_data=True)
    dm.nlu.train()
    return dm


@pytest.fixture
def dm_mocked(dm, mocker):
    mocker.patch.object(dm, "_ask_slot", return_value=None)
    mocker.patch.object(dm, "_submit_form", return_value=None)
    return dm


@pytest.mark.parametrize("utterance", ['', ' ', '��', ', ,,, ,', '.', '^_^', '©', ')'])
def test_empty_input_utterance(dm_mocked, utterance, session):
    assert session.intent_name is None
    dm_mocked.handle(
        create_request(uuid=session.uuid, utterance=utterance), session=session, response=None, app=object())
    assert session.intent_name == 'test_bot._internal_.dont_understand'


def test_very_large_request(dm_mocked, session):
    # DIALOG-910
    very_large_utterance = ' '.join(['очень длинный запрос'] * 1000000)
    dm_mocked.handle(
        create_request(uuid=session.uuid, utterance=very_large_utterance), session=session, app=object(), response=None)
    assert session.intent_name == 'test_bot._internal_.dont_understand'


@pytest.fixture(scope='module')
def feature_cache(app_config_session_scope, tmpdir_factory):
    tempdir = tmpdir_factory.mktemp('feature_cache')
    feature_cache_file = str(tempdir.join('feature_cache.pkl'))
    dm = DialogManager.from_config(app_config_session_scope, load_data=True)
    dm.nlu.train(feature_cache=feature_cache_file)
    yield feature_cache_file
    tempdir.remove()


def test_bin_cache_exists(feature_cache):
    assert os.path.exists(feature_cache + '.bin')


@pytest.mark.slowtest
@pytest.mark.parametrize('num_procs', (1, 2, 100))
@pytest.mark.parametrize('classifiers', (['none'], ['test_clf']))
@pytest.mark.parametrize('taggers', (False, True))
def test_train_with_feature_cache(mocker, monkeypatch, app_config, feature_cache, num_procs, classifiers, taggers):
    monkeypatch.setenv('VINS_NUM_PROCS', num_procs)
    samples_extractor_mock = mocker.patch.object(
        vins_core.nlu.samples_extractor.SamplesExtractor, 'process_item')
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train(feature_cache=feature_cache, classifiers=classifiers, taggers=taggers)
    assert samples_extractor_mock.call_count == 0


@pytest.mark.slowtest
@pytest.mark.parametrize('classifiers', (['none', 'test_fallback_clf'], ['test_clf', 'test_fallback_clf']))
@pytest.mark.parametrize('use_cache', (True, False))
def test_train_with_feature_cache_tagger_chunks(mocker, app_config, feature_cache, session, classifiers, use_cache):
    dm = DialogManager.from_config(app_config, load_data=True)
    for intent in dm.nlu.nlu_sources_data:
        for i, item in enumerate(dm.nlu.nlu_sources_data[intent]):
            if any(slot.name == 'slot1' for slot in item.slots):
                dm.nlu.nlu_sources_data[intent][i] = NluSourceItem(
                    item.text, item.original_text, item.slots, item.trainable_classifiers,
                    can_use_to_train_tagger=False
                )
    dm.nlu.train(classifiers=classifiers, taggers=True, feature_cache=feature_cache if use_cache else None)
    test_utt = dm.nlu.nlu_sources_data['test_bot.general.intent1'][-1]
    assert test_utt.slots[0].name == 'slot1'  # this one is skipped from training data

    mocker.patch.object(
        vins_core.nlu.flow_nlu.FlowNLU,
        '_predict_intents',
        return_value=[IntentCandidate(name='test_bot.general.intent1', score=1.0)]
    )
    result = dm.nlu.handle(Sample.from_string(test_utt.text), session).semantic_frames[0]
    # slot2 is recognized because slot1 is skipped
    assert len(result['slots'].keys()) == 1 and result['slots'].keys()[0] == 'slot2'


def test_intents_to_train_tagger(app_config, session, req_info):
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    results_with_slots = dm.nlu.handle(
        Sample.from_string('текст с параметром 1'), session, req_info=req_info
    ).semantic_frames
    assert any(
        len(result['slots']) > 0 and result['intent_name'].endswith('intent1')
        for result in results_with_slots
    )
    for intent, items in dm.nlu.nlu_sources_data.iteritems():
        new_items = [item for item in items if len(item.slots) == 0]
        dm.nlu.nlu_sources_data[intent] = new_items
    dm.nlu.train(intents_to_train_tagger='.*intent1')
    results_without_slots = dm.nlu.handle(
        Sample.from_string('текст с параметром 1'), session, req_info=req_info
    ).semantic_frames
    assert all(
        len(result['slots']) == 0 and result['intent_name'].endswith('intent1')
        for result in results_without_slots
    )


@pytest.fixture(scope='module')
def app_specific_test_utterance():
    return 'текст 1'


@pytest.fixture(scope='module')
def dm_with_app_specific_classifier(app_config_session_scope, app_specific_test_utterance):
    app_config = deepcopy(app_config_session_scope)
    app_config.nlu['intent_classifiers'].insert(0, {
        'model': 'data_lookup',
        'name': 'fixlist',
        'app': 'specific_app',
        'params': {
            'intent_texts': {
                'test_bot.general.micro1': [app_specific_test_utterance]
            }
        }
    })
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    return dm


@pytest.mark.parametrize('app, expected_intent, app_specific_classifier_is_not_called', [
    ('default', 'test_bot.general.intent1', True),
    ('specific_app', 'test_bot.general.micro1', False)
])
def test_app_specific_classifiers(
    dm_with_app_specific_classifier, app_specific_test_utterance, session, app, caplog,
    expected_intent, app_specific_classifier_is_not_called
):
    with caplog.at_level(logging.DEBUG):
        req_info = create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id=app))
        nlu_result = dm_with_app_specific_classifier.get_semantic_frames(
            Sample.from_string(app_specific_test_utterance), session, req_info
        )
        frame = nlu_result.semantic_frames[0]
        assert frame['intent_name'] == expected_intent
        if app_specific_classifier_is_not_called:
            assert 'Classifier fixlist is not allowed for current app' in caplog.text


@pytest.fixture(scope='module')
def dm_with_intent_trigger():
    app_config = AppConfig()
    app_config.parse_vinsfile('vins_core/test/test_data/test_app/Vinsfile.json')
    app_config.projects.append(Project.from_dict({
        'name': 'triggers',
        'intents': [{
            'intent': 'trigger',
            'trainable_classifiers': ['test_clf', 'test_fallback_clf'],
            'dm': {'data': {'name': 'other'}},
            'nlu': [{
                'source': 'data',
                'data': ['запускаем сценарий']
            }],
            'scenarios': [{
                'name': 'test_bot.general.intent1',
                'context': {'app': 'app_1'}
            }, {
                'name': 'test_bot.general.intent2',
                'context': {'experiments': 'the_experiment'}
            }, {
                'name': 'test_bot.general.intent2',
                'context': {'app': 'app_2', 'device_state': {'some_state': True}}
            }, {
                'name': 'test_bot.general.intent3',
                'context': {'app': 'app_3', 'prev_intents': 'test_bot.general.intent1'}
            }, {
                'name': 'test_bot.general.intent4',
                'context': {'app': 'app_3'}
            }, {
                'name': 'test_bot._internal_.dont_understand'
            }]
        }]
    }, base_name='test_bot'))
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()
    return dm


@pytest.mark.parametrize('test_phrase, req_info, output_scenario', [
    (
        'запускаем сценарий intent1 в app_1',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_1')),
        'test_bot.general.intent1'
    ),
    (
        'запускаем сценарий intent1 в app_1 при любом device_state',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_1'), device_state={'some_state': True}),
        'test_bot.general.intent1'
    ),
    (
        'запускаем сценарий intent2 в любом приложении, кроме app_1, если стоит флаг эксперимента',
        create_request(uuid=gen_uuid_for_tests(), experiments=('the_experiment',)),
        'test_bot.general.intent2'
    ),
    (
        'запускаем сценарий intent2 в app_2 при условии device_state',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_2'), device_state={'some_state': True}),
        'test_bot.general.intent2'
    ),
    (
        'запускаем сценарий dont_understand в приложении app_2, т.к. нет device_state',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_2')),
        'test_bot._internal_.dont_understand'
    ),
    (
        'запускаем сценарий dont_understand в приложении app_2, т.к. у device_state неправильное значение',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_2'), device_state={'some_state': False}),
        'test_bot._internal_.dont_understand'
    ),
    (
        'запускаем сценарий dont_understand в других аппах',
        create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='other_app')),
        'test_bot._internal_.dont_understand'
    )
])
def test_intent_triggers_scenarios(dm_with_intent_trigger, session, test_phrase, req_info, output_scenario):
    nlu_result = dm_with_intent_trigger.get_semantic_frames(
        Sample.from_string(test_phrase), session, req_info
    )
    frame = nlu_result.semantic_frames[0]
    assert frame['intent_name'] == output_scenario


@pytest.mark.parametrize('test_phrase, prev_intent, output_scenario', [
    (
        'запускаем сценарий intent3 в app_3 при условии предыдущего интента intent1',
        'test_bot.general.intent1',
        'test_bot.general.intent3'
    ),
    (
        'запускаем сценарий intent4 в app_3, потому что предыдущий интент не intent1',
        'test_bot.general.intent2',
        'test_bot.general.intent4'
    )
])
def test_intent_triggers_scenarios_with_prev_intent(
        dm_with_intent_trigger, session, test_phrase, prev_intent, output_scenario):
    req_info = create_request(uuid=gen_uuid_for_tests(), app_info=AppInfo(app_id='app_3'))
    session.change_intent(Intent(prev_intent))
    frame = dm_with_intent_trigger.get_semantic_frames(
        Sample.from_string(test_phrase), session, req_info
    ).semantic_frames[0]
    assert frame['intent_name'] == output_scenario


@pytest.mark.parametrize('add_custom_tagger_with_narrow_context', (True, False))
def test_mixed_model_tagger(session, app_config, add_custom_tagger_with_narrow_context, req_info):
    app_config.nlu['utterance_tagger'] = {
        'model': 'mixed',
        'params': {
            'default_config': {
                'model': 'crf',
                'features': ['word']
            }
        }
    }
    intent_cfg = register_intent('custom_tagger', 'Custom tagger NLG result', nlu=[
        '1 контекст "слот"(slot1)',
        '11 контекст "слот"(slot1)',
        '2 контекст "слот"(slot2)',
    ], trainable_classifiers=['test_clf'])
    if add_custom_tagger_with_narrow_context:
        intent_cfg['utterance_tagger'] = {
            'model': 'crf',
            'features': ['word'],
            'params': {'window_size': 1}
        }
    app_config.projects.append(Project.from_dict({
        'name': 'custom_taggers',
        'intents': [intent_cfg]
    }, base_name='test_bot'))
    dm = DialogManager.from_config(app_config, load_data=True)
    dm.nlu.train()

    test_phrase = '2 другой_контекст слот'
    frame = dm.nlu.handle(Sample.from_string(test_phrase), session, req_info=req_info).semantic_frames[0]
    if add_custom_tagger_with_narrow_context:
        # narrow context (custom tagger) cannot handle slot2 since prefix info is missed
        assert frame['slots'].keys()[0] == 'slot1'
    else:
        # with 2 tokens context (default tagger) prefix "2" is properly handled
        assert frame['slots'].keys()[0] == 'slot2'


@pytest.fixture
def app_config_multi_features(app_config):
    app_config.nlu['feature_extractors'].append({
        'id': 'postag',
        'type': 'postag'
    })
    app_config.nlu['utterance_tagger']['features'] = ['postag']
    return app_config


@pytest.fixture
def feature_cache_multi_features(tmpdir_factory, app_config_multi_features):
    tmp_dir = tmpdir_factory.mktemp('feature_cache_multi_features')
    feature_cache_file_obj = tmp_dir.join('feature_cache.pkl')
    feature_cache_file = str(feature_cache_file_obj)
    dm = DialogManager.from_config(app_config_multi_features, load_data=True)
    dm.nlu.train(feature_cache=feature_cache_file, taggers=False)
    yield feature_cache_file
    feature_cache_file_obj.remove()


def test_feature_cache_multi_features_all_features_presented(feature_cache_multi_features):
    feature_cache = SampleFeaturesCache(feature_cache_multi_features)
    any_sample_feature = feature_cache.collect_all().values()[0]
    assert set(any_sample_feature.sparse_seq) == {'word', 'postag'}


@pytest.mark.parametrize('taggers', (False, True))
def test_feature_cache_multi_features_models_are_trainable(
    feature_cache_multi_features, app_config_multi_features, session, req_info, taggers
):
    app_config_multi_features.projects[2].intents[0]._nlu_sources.config.append({
        'source': 'data',
        'data': ['добавим новых фраз', 'чтобы проверить, что кеш "не ломается"(expected)']
    })
    dm = DialogManager.from_config(app_config_multi_features, load_data=True)
    dm.nlu.train(feature_cache=feature_cache_multi_features, taggers=taggers)
    result = dm.nlu.handle(Sample.from_string('не ломается'), session, req_info=req_info).semantic_frames[0]
    if not taggers:
        assert len(result['slots']) == 0
    else:
        assert result['slots'].keys()[0] == 'expected'


def test_get_semantic_frames():
    dm = DialogManager(
        intents=[],
        intent_to_form={},
        nlu=BaseNLU(fst_parser_factory=FstParserFactory(None, {}), fst_parsers=[]),
        nlg=None,
        nlu_post_processor=IntentGroupNluPostProcessor(
            ChainedNluPostProcessor([SortNluPostProcessor(), FirstFrameChoosingNluPostProcessor()])
        )
    )
    dm.nlu.handle = mock.MagicMock(return_value=NLUHandleResult(
        semantic_frames=[
            {'intent_candidate': IntentCandidate(name='intent_1', score=0.7),
             'intent_name': 'intent_1', 'confidence': 0.7},
            {'intent_candidate': IntentCandidate(name='intent_2', score=0.5),
             'intent_name': 'intent_2', 'confidence': 0.5},
            {'intent_candidate': IntentCandidate(name='intent_2', score=0.8),
             'intent_name': 'intent_2', 'confidence': 0.8},
            {'intent_candidate': IntentCandidate(name='intent_3', score=0.6),
             'intent_name': 'intent_3', 'confidence': 0.6},
            {'intent_candidate': IntentCandidate(name='intent_3', score=0.9),
             'intent_name': 'intent_3', 'confidence': 0.9},
            {'intent_candidate': IntentCandidate(name='intent_3', score=0.3),
             'intent_name': 'intent_3', 'confidence': 0.3}
        ],
        sample_features=None
    ))
    frames = dm.get_semantic_frames(
        sample=None, session=None, req_info=create_request('uuid1')
    ).semantic_frames
    assert frames == [
        {'intent_candidate': IntentCandidate(name='intent_1', score=0.7), 'intent_name': 'intent_1', 'confidence': 0.7},
        {'intent_candidate': IntentCandidate(name='intent_2', score=0.8), 'intent_name': 'intent_2', 'confidence': 0.8},
        {'intent_candidate': IntentCandidate(name='intent_3', score=0.9), 'intent_name': 'intent_3', 'confidence': 0.9}
    ]
