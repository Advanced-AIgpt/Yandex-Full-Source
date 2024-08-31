# coding: utf-8
from __future__ import unicode_literals

import pytest
import numpy as np

from marisa_trie import BytesTrie

from vins_core.config.app_config import Project, Intent, AppConfig, Entity
from vins_core.dm.form_filler.models import Form, Slot, Handler, Event
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.ner.fst_custom import NluFstCustom
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.ner.fst_presets import FstParserFactory, PARSER_RU_BASE_PARSERS
from vins_sdk.app import VinsApp
from vins_sdk.connectors import TestConnector
from vins_core.utils.data import TarArchive, get_resource_full_path
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_core.nlu.sample_processors.wizard import WizardSampleProcessor
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.config.app_config import NluSourcesConfig
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.dm.request import create_request
from vins_core.utils.misc import gen_uuid_for_tests


def pytest_sessionfinish(session, exitstatus):
    """ Tensorflow issue workaround
    https://github.com/tensorflow/tensorflow/issues/3388
    """
    from keras import backend as K
    K.clear_session()


def _get_simple_app(test_entity_parser, num_classifiers=1):
    intent_1 = Intent(
        name='intent_1',
        trainable_classifiers=["test"],
        dm_form=Form(
            name='intent_1',
            slots=[
                Slot(name='thing', types=['string'], import_tags=['thing'], share_tags=['thing'], optional=True),
            ],
            events=[
                Event(name='submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'say_1'}
                )])
            ]
        ),
        nlu_sources=NluSourcesConfig([{'source': 'data', 'data': [
            "что такое",
            "что такое 'яблоко'(thing)"
        ]}]),
        nlg_sources="""
            {% phrase say_1 %}
                {{ form.thing }} это ...
            {% endphrase %}
        """
    )
    intent_2 = Intent(
        name='intent_2',
        trainable_classifiers=["test"] if num_classifiers == 1 else ['test', 'test_2'],
        dm_form=Form(
            name='intent_2',
            slots=[
                Slot(name='thing', types=['string'], import_tags=['thing'], share_tags=['thing'], optional=True),
            ],
            events=[
                Event(name='submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'say_2'}
                )])
            ]
        ),
        nlu_sources=NluSourcesConfig([{'source': 'data', 'data': [
            "какого цвета",
            "какого цвета 'яблоко'(thing)",
            "а какого цвета"
        ]}]),
        nlg_sources="""
            {% phrase say_2 %}
              {{ form.thing }} ... цвета
            {% endphrase %}
        """
    )
    intent_3 = Intent(
        name='custom_entity_test',
        trainable_classifiers=["test"] if num_classifiers == 1 else ['test', 'test_2'],
        dm_form=Form(
            name='custom_entity_test',
            slots=[
                Slot(name='what', types=['test_entity'], optional=False),
            ],
            events=[
                Event(name='submit', handlers=[Handler(
                    handler='callback',
                    name='nlg_callback',
                    params={'phrase_id': 'resp'}
                )])
            ]
        ),
        nlu_sources=NluSourcesConfig([{'source': 'data', 'data': [
            "включи 'тест'(what)",
            "включи 'test'(what)",
            "включи 'not test'(what)",
            "включи 'что угодно'(what)"
        ]}]),
        nlg_sources="""
            {% phrase resp %}
              {{ form.thing }} ... цвета
            {% endphrase %}
        """,
        total_fallback=True
    )

    entities = [
        Entity(name='test_entity',
               samples={'test': ['test', 'тест'], 'not_test':['not test']})
    ]
    app_cfg = AppConfig([
        Project(name='test_project', intents=[intent_1, intent_2, intent_3], entities=entities)
    ])
    intent_classifiers = [{
        'model': 'maxent',
        'name': 'test',
        'features': ['classifier', 'word', 'bigram', 'ner', 'postag', 'lemma']
    }]
    if num_classifiers > 1:
        intent_classifiers.insert(0, {
            'model': 'rnn',
            'name': 'test_2',
            'features': ['word'],
            'params': {'intent_conditioned': True}
        })
    app_cfg.nlu = {
        'feature_extractors': [
            {
                'id': 'classifier',
                'type': 'classifier',
                'model': 'charcnn',
                'model_file': 'resource://query_subtitles_charcnn',
                'feature': 'scores'
            },
            {
                'id': 'word',
                'type': 'ngrams',
                'n': 1
            },
            {
                'id': 'bigram',
                'type': 'ngrams',
                'n': 2
            },
            {
                'id': 'ner',
                'type': 'ner',
            },
            {
                'id': 'postag',
                'type': 'postag'
            },
            {
                'id': 'lemma',
                'type': 'lemma'
            }
        ],
        'intent_classifiers': intent_classifiers,
        'utterance_tagger': {
            'model': 'crf',
            'name': 'tagger',
            'features': ['word', 'ner', 'lemma'],
            'params': {'intent_conditioned': True}
        },
        'fst': {
            'resource': 'resource://fst',
            'parsers': PARSER_RU_BASE_PARSERS
        },
        'samples_extractor': {
            'pipeline': [{
                'name': 'normalizer',
                'normalizer': DEFAULT_RU_NORMALIZER_NAME
            }]
        }
    }

    dm = DialogManager.from_config(app_cfg, load_data=True)
    dm.app_cfg = app_cfg
    dm.nlu.set_entity_parsers({'test_entity': test_entity_parser})
    dm.nlu.train()

    app = VinsApp(app_id='test_app', dm=dm)
    connector = TestConnector(vins_app=app)
    return connector


@pytest.fixture(scope='package')
def parser_factory(app_config_session_scope):
    factory = FstParserFactory.from_config(app_config_session_scope.nlu.get('fst', {}))
    factory.load()
    return factory


@pytest.fixture(scope='package')
def parser_base(parser_factory):
    return parser_factory.create_parser(PARSER_RU_BASE_PARSERS)


@pytest.fixture(scope='package')
def parser(parser_factory, app_config_session_scope):
    return parser_factory.create_parser(app_config_session_scope.nlu.get('fst', {}).get('parsers', []))


@pytest.fixture(scope='package')
def test_entity_parser():
    resource = get_resource_full_path('resource://custom_enitities/test_entities.tar.gz')
    with TarArchive(resource) as arch:
        return NluFstCustom.load_from_archive('test_entity', archive=arch)


@pytest.fixture(scope='package')
def simple_app(test_entity_parser):
    return _get_simple_app(test_entity_parser)


@pytest.fixture(scope='package')
def simple_app_with_cascade(test_entity_parser):
    return _get_simple_app(test_entity_parser, num_classifiers=2)


@pytest.fixture(scope='function')
def function_scoped_simple_app(test_entity_parser):
    return _get_simple_app(test_entity_parser)


@pytest.fixture(scope='function')
def app_config():
    app_config = AppConfig()
    app_config.parse_vinsfile('vins_core/test/test_data/test_app/Vinsfile.json')
    return app_config


@pytest.fixture(scope='function')
def app_config_for_metric(app_config):
    config = app_config
    project_dict = {
        "name": "general",
        "intents": [
            {
                "intent": "the_funniest_intent_ever",
                "negative_sampling_from": "the_most_boring_intent_ever|the_ugliest_intent_ever",  # the default
                "positive_sampling": True,
                "dm": {"data": {"name": "dummy_dm"}}
            },
            {
                "intent": "the_most_boring_intent_ever",
                "positive_sampling": False,
                "dm": {"data": {"name": "dummy_dm"}}
            },
            {
                "intent": "the_ugliest_intent_ever",
                "negative_sampling_from": "the_funniest_intent_ever",   # don't compare it with the boring one!
                "dm": {"data": {"name": "dummy_dm"}}
            }
        ]
    }
    project = Project.from_dict(project_dict)
    config.projects.append(project)
    return config


@pytest.fixture(scope='function')
def app_with_intents_for_metric(app_config_for_metric):
    return VinsApp(app_conf=app_config_for_metric)


@pytest.fixture(scope='session')
def app_config_session_scope():
    return app_config()


@pytest.fixture(scope='session')
def nlu_demo_data():
    _intent_general = {
        'goodbye': [
            '# - классификация -',
            'пока',
            'покедова',
            'до свидания'
        ],
        'greeting': [
            'привет',
            'приветик',
            'здорова',
            'хай'
        ],
        'how_are_you': [
            'как дела\n',
            'у тебя все ок?',
            'ты норм?'
        ],
        'thanks': [
            'спасиб',
            'спасибо',
            'благодарю',
            'ты лучший',
            'ты лучшая']
    }

    _intents = {
        'alarm': [
            'будильник на "8:30"(when)',
            'поставь будильник на "6 утра"(when)',
            'разбуди меня "завтра в без 15 4"(when) обязательно',
            'разбуди меня в "9 вечера"(when)',
            'разбудить'
        ],
        'taxi': [
            'заберите "иванова ивана"(client) из "иваново"(location_from)',
            'заберите "петрова петра"(client) из "набережных челнов"(location_from)',
            'заберите "бэтмена"(client) из "москвы"(location_from)',
            'заберите "моего брата"(client) из "томска"(location_from)',
            "едем из 'набережных челнов'(location_from)",
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машина до "рокоссовского дом 16"(location_to)',
            'машину от "тверской улицы"(location_from) в "без 15 6 вечера"(when) не дороже "500 рублей"(price)',
            '"город Москва улица Тимура Фрунзе дом 16/1"(location_from)',
            '"город Москва улица Льва Толстого дом 16"(location_from)',
            'нужно такси на "ленинградский проспект дом 9 корпус 1"(location_from) на "6 часов утра"(when)',
            'такси от "зеленого проспекта"(location_from) до "чистопрудного бульвара"(location_to)',
            'из "москвы"(location_from) до "воронежа"(location_to) "завтра"(when)',
            "хочу заказать такси на '8 утра'(when)",
            "закажи мне такси на 'улицы маршала неделина дом 6'(location_from)",
            "мне нужно такси в 'москву'(location_to)",
            'нужна машина до "проспекта вернадского 9"(location_to) на "полвосьмого утра"(when)',
            'машину от "площади гагарина"(location_from) до "площади ленина"(location_to) на "10"(when)',
            'такси до "одинцово"(location_to)'

            "закажи мне такси",
            "мне нужно такси",
            "закажи такси",
            "закажи мне такси",
            "мне нужна тачка",
            "отвезите меня",
            "мне нужно ехать",
            "как можно быстрее закажите такси"
        ]
    }
    _intents.update(_intent_general)
    return _intents


@pytest.fixture(scope='session')
def dummy_embeddings(tmpdir_factory):
    tmpdir = tmpdir_factory.mktemp('test_data')
    out_file = str(tmpdir.join('test_features_dummy_word2vec'))

    def to_bytes(data):
        return np.array(data, dtype=np.float32).tobytes()

    trie = BytesTrie([
        ('улица', to_bytes([1, 2, 3])),
        ('льва', to_bytes([-1, -2, -3])),
        ('толстого', to_bytes([3, 6, 9])),
        ('погода', to_bytes([1, 1, 1])),
    ])

    trie.save(out_file)
    yield out_file
    tmpdir.remove()


@pytest.fixture(scope='session')
def dummy_embeddings_2(tmpdir_factory):
    tmpdir = tmpdir_factory.mktemp('test_data')
    out_file = str(tmpdir.join('test_features_dummy_word2vec_2'))

    trie = BytesTrie([
        ('привет', np.array([.1, .2, .3], dtype=np.float32).tobytes()),
        ('такси', np.array([-.1, -.2, -.3], dtype=np.float32).tobytes()),
        ('будильник', np.array([.3, .6, .9], dtype=np.float32).tobytes()),
        ('спасибо', np.array([.1, .1, .1], dtype=np.float32).tobytes())
    ])
    trie.save(out_file)
    yield out_file
    tmpdir.remove()


@pytest.fixture(scope='package')
def custom_ner_currency():
    name = 'custom_currency'
    resource = get_resource_full_path('resource://custom_enitities/test_entities.tar.gz')
    with TarArchive(resource) as arch:
        parser = NluFstCustom.load_from_archive(
            fst_name=name,
            archive=arch
        )
    return {name: parser}


@pytest.fixture(scope='package')
def normalize_sample_processor():
    return NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)


@pytest.fixture(scope='package')
def normalizing_samples_extractor(normalize_sample_processor):
    return SamplesExtractor(pipeline=[normalize_sample_processor])


@pytest.fixture(scope='package')
def normalizing_samples_extractor_config(normalize_sample_processor):
    return {
        'pipeline': [{
            'name': 'normalizer',
            'normalizer': DEFAULT_RU_NORMALIZER_NAME
        }]}


@pytest.fixture(scope='package')
def samples_extractor(normalizing_samples_extractor):
    return normalizing_samples_extractor


@pytest.fixture(scope='package')
def samples_extractor_config(normalizing_samples_extractor_config):
    return normalizing_samples_extractor_config


@pytest.fixture(scope='package')
def samples_extractor_with_wizard(normalize_sample_processor):
    pipeline = [normalize_sample_processor, WizardSampleProcessor()]
    return SamplesExtractor(pipeline=pipeline)


@pytest.fixture(scope='package')
def samples_extractor_with_wizard_config(normalizing_samples_extractor_config):
    config = normalizing_samples_extractor_config
    config['pipeline'].append({'name': 'wizard'})
    return config


@pytest.fixture(scope='package')
def nlu_demo_samples(nlu_demo_data, samples_extractor):
    return {
        intent: samples_extractor(FuzzyNLUFormat.parse_iter(texts).items)
        for intent, texts in nlu_demo_data.iteritems()
    }


@pytest.fixture(scope='package', params=[None, 'sorting_transition_model'])
def req_info(request):
    return create_request(gen_uuid_for_tests(), experiments=request.param)
