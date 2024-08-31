# coding: utf-8
from __future__ import unicode_literals

import pytest
import mock
import copy

from vins_core.common.sample import Sample
from vins_core.ner.fst_presets import PARSER_RU_BASE_PARSERS, FstParserFactory

from vins_core.common.slots_map_utils import get_slot_value
from vins_core.nlu.anaphora.resolver import AnaphoraResolver
from vins_core.nlu.classifier import Classifier
from vins_core.nlu.flow_nlu import FlowNLU, _DEFAULT_FALLBACK_THRESHOLD
from vins_core.nlu.flow_nlu_factory.transition_model import create_transition_model, MarkovTransitionModel
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.nlu.sample_processors.normalize import FstNormalizerError, NormalizerEmptyResultError
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.dm.formats import FuzzyNLUFormat, NluSourceItem
from vins_core.dm.intent import Intent
from vins_core.dm.session import Session
from vins_core.config import app_config
from vins_core.nlu.features.base import SampleFeatures


class StupidAnaphoraResolver(AnaphoraResolver):
    def __init__(self, matcher=None, samples_extractor=None):
        super(StupidAnaphoraResolver, self).__init__(matcher, samples_extractor)

    def __call__(self, *args, **kwargs):
        return 'закажи такси в москву'


@pytest.fixture
def taxi_order_data():
    _intents = {
        'other': [u"ahslkjfhksdf"],
        'taxi_order': [
            "закажи такси",
            "закажи мне такси",
            "мне нужна тачка",
            "отвезите меня",
            "мне нужно ехать",

            "закажи мне такси на '8 утра'(when)",
            "мне нужна машина на 'улицу маршала неделина дом 6'(location_from) на '7 утра'(when)",
            "мне нужно такси от 'улицу маршала неделина дом 6'(location_from) до 'вавилова 10'(location_to)",

            "мне нужно такси и чтобы был 'кондиционер'(requirements.conditioner)",
            "я хочу заказать такси и чтобы 'покурить'(requirements.smoking) можно было",

            "мне нужен 'кондиционер'(requirements.conditioner)",
            "а 'покурить'(requirements.smoking) у вас можно",
            "и чтобы можно было 'покурить'(requirements.smoking)",
            "со мной поедет 'ребёнок'(requirements.child) 5 лет",
            "нужно 'детское кресло'(requirements.child)",
            "а у вашего водителя будет 'детское кресло'(requirements.child)",
            "мне обязательно 'детское кресло'(requirements.child)",

            "машина от 'улицы ивана франко дом 2'(location_from) до 'кутузовский проспект дом 2'(location_to)",
            "такси от 'проспекта вернадского'(location_from) до 'реутова'(location_to) к '9 утра'(when)",
            "такси на 'тверскую улицу 6'(location_from) 'завтра в 16 часов вечера'(when)",

            "машину от 'дегтярного переулка 8'(location_from) до 'волгоградского проспекта 21'(location_to) 'без 5 5 утра'(when)"  # noqa
            "мне нужно такси в 'москву'(location_to)",
            "от 'проспекта мира 23 корпус 1'(location_from) до 'балашихи'(location_to)",

            "закажи мне такси на 'улицы маршала неделина дом 6'(location_from)",
            "заказ такси 'через 2 часа'(when)"
        ]
    }
    return _intents


def samples_extractor_config():
    config = {
        "pipeline": [
            {
                "name": "normalizer",
                "normalizer": "normalizer_ru"
            }
        ]
    }
    return config


@pytest.fixture(name='samples_extractor_config', scope='module')
def samples_extractor_config_fixture():
    return samples_extractor_config()


@pytest.fixture(scope='module')
def samples_extractor_en_config():
    config = {
        "pipeline": [
            {
                "name": "normalizer",
                "normalizer": "normalizer_en"
            }
        ]
    }
    return config


def default_params():
    return dict(
        feature_extractors=[
            {'type': 'ngrams', 'id': 'word', 'n': 1},
            {'type': 'ngrams', 'id': 'bigram', 'n': 2},
            {'type': 'lemma', 'id': 'lemma'},
            {'type': 'ner', 'id': 'ner'},
            {'type': 'postag', 'id': 'postag'}
        ],
        intent_classifiers=[{
            'name': 'intent_classifier_0',
            'model': 'maxent',
            'l2reg': 1e-2,
            'features': ['word', 'bigram', 'lemma', 'ner', 'postag']
        }],
        fallback_intent_classifiers=[{
            'name': 'intent_classifier_1',
            'model': 'maxent',
            'l2reg': 1e-2,
            'features': ['word']
        }],
        utterance_tagger={
            'model': 'crf',
            'features': ['word', 'lemma', 'ner'],
            'params': {'intent_conditioned': True}
        }
    )


@pytest.fixture(name='default_params')
def default_params_fixture():
    return default_params()


def nlu(nlu_demo_data, samples_extractor_config, parser_factory, default_params):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    fallback_threshold = 0.75
    templates = app_config.AppConfig.default_nlu_templates()
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data,
            name=intent_name,
            templates=templates
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=fallback_threshold,
            tagger_data_keys={intent_name},
            trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
    f.add_intent('dont_understand', fallback_threshold=fallback_threshold, total_fallback=True)
    return f


@pytest.fixture(name='nlu', scope='function')
def nlu_fixture(nlu_demo_data, samples_extractor_config, parser_factory, default_params):
    return nlu(nlu_demo_data, samples_extractor_config, parser_factory, default_params)


def nlu_train(data, sample_extractor, parser_factory, **kwargs):
    f = FlowNLU(
        samples_extractor=samples_extractor_config(),
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **kwargs)
    f.load()
    fallback_threshold = kwargs.get('threshold', 0.75)
    templates = app_config.AppConfig.default_nlu_templates()
    for intent_name, data in data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data,
            name=intent_name,
            templates=templates,
            trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=fallback_threshold,
            tagger_data_keys={intent_name}
        )
    f.add_intent('dont_understand', fallback_threshold=fallback_threshold, total_fallback=True)
    f.train()

    return f


@pytest.fixture(scope='module')
def nlu_obj(nlu_demo_data, samples_extractor, parser_factory):
    _intents = nlu_demo_data
    parsed = FuzzyNLUFormat.parse_one(_intents['taxi'][0])
    assert parsed.text == 'заберите иванова ивана из иваново'
    assert parsed.slots[0].name == 'client'
    assert parsed.slots[0].begin == 9
    assert parsed.slots[0].end == 22

    return nlu_train(_intents, samples_extractor, parser_factory, **default_params())


@pytest.fixture(scope='module')
def session():
    return Session(app_id='123', uuid='123')


def test_handle_bad_symbols(nlu_obj, session, samples_extractor, req_info):
    r = nlu_obj.handle(samples_extractor(['125÷11.18'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'dont_understand'


def test_train_bad_symbols(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'bad': [
            '125÷11.18',
            'посмотрим død snø'
        ],
        'test': [
            'тестируем "abc"(slot)',
            'тестируем "def"(slot)',
        ]
    }
    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)
    r = f.handle(samples_extractor(['125÷11.18'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'bad'


def test_FuzzyNLU(nlu_obj, session, samples_extractor, req_info):

    r = nlu_obj.handle(
        samples_extractor(['нужна машина до проспекта буденного шесть на полпятого вечера'])[0],
        session,
        req_info=req_info
    ).semantic_frames[0]
    assert r['intent_name'] == 'taxi'
    assert 'location_to' in r['slots']
    assert r['slots']['location_to'][0]['substr'] == 'проспекта буденного 6'
    location_to = get_slot_value(r['slots']['location_to'][0], 'GEO')
    assert location_to['house'] == 6
    assert location_to['street'] == 'буденного'


def test_force_intent(nlu_obj, session, samples_extractor, req_info):
    r = nlu_obj.handle(samples_extractor(['привет'])[0], session, force_intent='taxi',
                       req_info=req_info).semantic_frames
    assert set(sum(map(lambda item: item['slots'].keys(), r), [])) == {
        'location_from', 'location_to', 'price', 'when', 'client'
    }


def test_unregistered_force_intent(nlu_obj, session, samples_extractor, req_info):
    force_intent = 'unregistered_force_intent'
    try:
        nlu_obj.handle(samples_extractor(['привет'])[0], session, force_intent=force_intent,
                           req_info=req_info).semantic_frames
        assert False, 'Forcing invalid intent must throw an exception. This code must be unreachable'
    except ValueError as e:
        assert e.message == 'Invalid force_intent experiment. Cannot force non-existent intent "unregistered_force_intent"'


def test_no_entity(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'unk': [
            'asfadf'
        ],
        'test': [
            'тестируем "abc"(slot)',
            'тестируем "def"(slot)',
        ]
    }
    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)
    r = f.handle(samples_extractor(['тестируем hig'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'test'
    assert r['slots']['slot'][0]['substr'] == 'hig'


def test_multi_tokens(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'unk': [
            'asdsdf'
        ],
        'test': [
            'поставь "дождь за окном"(slot)'
        ]
    }
    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)
    r = f.handle(samples_extractor(['поставь дождь за окном'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'test'
    assert r['slots']['slot'][0]['substr'] == 'дождь за окном'


def test_on_today(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'unk': [
            'asdsdf'
        ],
        'test': [
            'задачи "на сегодня"(when)'
        ]
    }
    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)
    r = f.handle(samples_extractor(['задачи на сегодня'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'test'
    assert r['slots']['when'][0]['substr'] == 'на сегодня'
    when = get_slot_value(r['slots']['when'][0], 'DATETIME')
    assert when == {'days_relative': True, 'days': 0}


def test_geo_datetime_100(session, samples_extractor, parser_factory, taxi_order_data, default_params, req_info):

    f = nlu_train(taxi_order_data, samples_extractor, parser_factory, **default_params)
    r = f.handle(
        samples_extractor(["закажи мне такси от улицы льва толстого дом 16 до одинцово через 15 минут"])[0],
        session,
        req_info=req_info
    ).semantic_frames[0]
    assert r['intent_name'] == 'taxi_order'
    assert r['slots']['location_from'][0]['substr'] == 'улицы льва толстого дом 16'
    assert r['slots']['location_to'][0]['substr'] == 'одинцово'
    assert r['slots']['when'][0]['substr'] == 'через 15 минут'

    location_from = get_slot_value(r['slots']['location_from'][0], 'GEO')
    assert location_from == {'house': 16, 'street': 'льва толстого'}
    location_to = get_slot_value(r['slots']['location_to'][0], 'GEO')
    assert location_to == {'city': {'id': 10743, 'name': 'Одинцово'}}
    when = get_slot_value(r['slots']['when'][0], 'DATETIME')
    assert when == {'minutes': 15, 'minutes_relative': True}


def test_client_100(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'other': ['dsfhk'],
        'thin_client': [
            "клиент",
            "Тонкий клиент",
            "Citrix",
            "Цитрикс",
            "МАРМ на",
            "АРМ",
            "ТК"
        ],
        'time': [
            "'8:40'(when)"
        ]
    }

    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)

    r = f.handle(samples_extractor(["МАРМ на 10 утра"])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'thin_client'
    assert get_slot_value(r, 'NUM') == 10
    assert get_slot_value(r, 'DATETIME') == {u'hours': 10, u'minutes': 0}


def test_slots_map_list(nlu_obj, session, samples_extractor, req_info):
    r = nlu_obj.handle(samples_extractor(["привет поставь будильник на 6 утра"])[0], session,
                       req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'alarm'
    assert r['slots']['when'][0]['substr'] == '6 утра'
    when = get_slot_value(r['slots']['when'][0], 'DATETIME')
    assert when == {'hours': 6, 'minutes': 0}


def test_format_short(nlu_obj, session, samples_extractor, req_info):
    utt = 'заберите ддлофыа ддоыпова из питера'
    r = nlu_obj.handle(samples_extractor([utt])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'taxi'
    assert r['slots']['client'][0]['substr'] == 'ддлофыа ддоыпова'
    assert r['slots']['location_from'][0]['substr'] == 'питера'
    location_from = get_slot_value(r['slots']['location_from'][0], 'GEO')
    assert location_from == {u'city': {'id': 2, 'name': 'Санкт-Петербург'}}


def test_format_substring(nlu_obj, session, samples_extractor, req_info):
    utt = 'заберите ддлофыа ддоыпова из нижнего новгорода'
    r = nlu_obj.handle(samples_extractor([utt])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'taxi'
    assert r['slots']['client'][0]['substr'] == 'ддлофыа ддоыпова'
    assert r['slots']['location_from'][0]['substr'] == 'нижнего новгорода'
    location_from = get_slot_value(r['slots']['location_from'][0], 'GEO')
    assert location_from == {'city': {'id': 47, 'name': 'Нижний Новгород'}}


def test_crash_1(samples_extractor):
    d = u'нет я ничего не хочу подключить я хочу уточнить куда делись деньги сегодня на счет я положила 200 рублей мне нарисовали долг который был 68 рублей там щас у меня на данный момент 10 рублей куда делись деньги я скорее'  # noqa
    p = FuzzyNLUFormat.parse_iter([d], name='intent')
    samples_extractor(p.items)


def test_crash_2(samples_extractor):
    d = u'+'
    p = FuzzyNLUFormat.parse_iter([d], name='intent')
    samples_extractor(p.items)


def test_normalized_utt(nlu_obj, session, samples_extractor, req_info):
    utt = "в две тысячи тридцать пятом году, до нашей эры: "
    r = nlu_obj.handle(samples_extractor([utt])[0], session, return_normalized_utt=True,
                       req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'dont_understand'
    assert r['normalized_utt'] == 'в 2035 году до нашей эры'
    entity = get_slot_value(r, 'DATETIME')
    assert entity['years'] == 2035


def test_fake_utts(session, samples_extractor, parser_factory, default_params):
    intents = {
        'gggg': [
            'asdsdf'
        ],
        'fake': [
            'Мой договор на интернет 000000000, на телефон 0000000000',
            '12-е февраля 1981.',
            'туризм. 004 ооо Панорама ,москва 000205-72364-010716',
            '=0 2>9=5 :0: =0 2>9=5 D8;L< A<>B@@5BL >=;09= 15A;0B=>',
            'экстрасенсы ведут расследование 30 07 2016 скачать торрент',
            'not fake'
        ]
    }
    nlu_train(intents, samples_extractor, parser_factory, **default_params)


def test_patterns(session, samples_extractor, parser_factory, default_params, req_info):
    intents = {
        'unk': [
            'asdsdf'
        ],
        'test': [
            'позвони "@first_name_ru(100)"(slot)'
        ]
    }
    f = nlu_train(intents, samples_extractor, parser_factory, **default_params)
    r = f.handle(samples_extractor(['позвони егору'])[0], session, req_info=req_info).semantic_frames[0]
    assert r['intent_name'] == 'test'
    assert r['slots']['slot'][0]['substr'] == 'егору'


def test_nbest(session, samples_extractor, nlu_demo_data, parser_factory, default_params, req_info):
    params = default_params
    params['utterance_tagger']['nbest'] = 3
    nlu_obj = nlu_train(nlu_demo_data, samples_extractor, parser_factory, **params)
    utt = "закажи мне такси от улицы льва толстого дом 16 до одинцово через 15 минут"
    r = nlu_obj.handle(samples_extractor([utt])[0], session, nbest=3, req_info=req_info).semantic_frames[:3]

    assert all(r_['intent_name'] == 'taxi' for r_ in r)

    assert set(r[0]['slots'].keys()) == {'location_from', 'location_to', 'when'}
    assert r[0]['slots']['location_from'][0]['substr'] == 'улицы льва толстого дом 16'
    assert r[0]['slots']['location_to'][0]['substr'] == 'одинцово'
    assert r[0]['slots']['when'][0]['substr'] == 'через 15 минут'

    assert set(r[1]['slots'].keys()) == {'location_from', 'when'}
    assert r[1]['slots']['location_from'][0]['substr'] == 'улицы льва толстого дом 16'
    assert r[1]['slots']['when'][0]['substr'] == 'через 15 минут'

    assert set(r[2]['slots'].keys()) == {'location_from', 'location_to', 'when'}
    assert r[2]['slots']['location_from'][0]['substr'] == 'улицы льва толстого дом 16 до'
    assert r[2]['slots']['location_to'][0]['substr'] == 'одинцово'
    assert r[2]['slots']['when'][0]['substr'] == 'через 15 минут'

    assert r[0]['tagger_score'] > r[1]['tagger_score']
    assert r[1]['tagger_score'] > r[2]['tagger_score']


@pytest.mark.parametrize("wizard_response, has_entity", [
    (None, True),
    ({'rules': {}}, True),
    ({'rules': {'AliceAnaphoraSubstitutor': {}}}, True),
    ({'rules': {'AliceAnaphoraSubstitutor': {'Substitution': [
        {'RewrittenRequest': 'закажи такси в москву', 'IsRewritten': True}
    ]}}}, True),
    ({'rules': {'AliceAnaphoraSubstitutor': {'Substitution': []}}}, False),
    ({'rules': {'AliceAnaphoraSubstitutor': {'Substitution': [
        {'RewrittenRequest': 'закажи такси туда'}
    ]}}}, False),
    ({'rules': {'AliceAnaphoraSubstitutor': {'Substitution': [
        {'RewrittenRequest': 'закажи такси туда', 'IsRewritten': False}
    ]}}}, False),
    ({'rules': {'AliceAnaphoraSubstitutor': {'Substitution': [
        {'RewrittenRequest': 'закажи такси туда', 'IsRewritten': True}
    ]}}}, False)
])
def test_anaphora_slot_matching(session, samples_extractor_with_wizard, normalizing_samples_extractor,
                                parser_factory, nlu_demo_data, default_params, req_info,
                                wizard_response, has_entity):
    params = default_params
    params['utterance_tagger']['nbest'] = 3
    params['anaphora_config'] = {'intents': ['taxi']}
    nlu_obj = nlu_train(nlu_demo_data, normalizing_samples_extractor, parser_factory, **params)
    nlu_obj._anaphora_resolver = StupidAnaphoraResolver()

    features = None
    if wizard_response:
        features = {'wizard': wizard_response}

    sample = samples_extractor_with_wizard(['закажи такси туда'], features=features)[0]
    frame = nlu_obj.handle(sample, session, req_info=req_info).semantic_frames[0]

    assert frame['intent_name'] == 'taxi'
    if not has_entity:
        assert not frame['slots']['location_to']
    else:
        assert len(frame['slots']['location_to'][0]['entities']) == 1
        entity = frame['slots']['location_to'][0]['entities'][0]
        assert entity.type == 'GEO' and entity.value['city']['name'] == 'Москва'


def test_flow_nlu_demo(session, samples_extractor_config, samples_extractor,
                       parser_factory, nlu_demo_data, default_params, req_info):
    # First create NLU object as usual
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data, name=intent_name, trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=0.8,
            tagger_data_keys={intent_name}
        )

    # fallback behaviour needs special intent
    # Fallback is a predictor that gives 1 if all output scores are bellow threshold
    f.add_intent('dont_understand', fallback_threshold=0.8, total_fallback=True)
    f.train()

    def recognized(text):
        return f.handle(samples_extractor([text])[0], session, req_info=req_info).semantic_frames[0]['intent_name']

    # Check intents
    assert recognized('нужна машина до проспекта буденного шесть на полпятого вечера') == 'taxi'
    assert recognized('как сделать дверь') == 'dont_understand'
    assert recognized('ты в порядке?') == 'dont_understand'

    # ================== NLU FLOW =======================
    # Ok, now we want to go deeper and split dont_understand on 'query' and 'chatter'
    # First we load pretrained classifier
    new_classifier_name = 'when-dont-understand-evoked'
    classifier = create_token_classifier(
        model='data_lookup',
        name=new_classifier_name,
        intent_texts={
            'search': ['как сделать дверь'],
            'gc': ['ты в порядке']
        },
        intent_infos={
            'search': [None],
            'gc': [None]
        }
    )

    # then add this predictor to pool...
    f.add_classifier(
        classifier=classifier
    )

    # ...and specify predictors that activate target intents
    # predictors can use classifier outputs and infer scores that multiplied together for each intent
    # when 'predictor_name' is omitted, 'root_predictor' (VINS-like trainable predictor) is assumed
    # in this example likelihood for intent "i" from "new_classifier_name"
    # will be scaled with 1 if fallback in root predictor happens and 0 otherwise
    for new_intent_name in classifier.classes:
        f.add_intent(
            new_intent_name,
            fallback_threshold=0.8,
            tagger_data_keys={new_intent_name}
        )

    # now check what happens
    assert recognized('как сделать дверь') == 'search'
    assert recognized('ты в порядке?') == 'gc'


def test_flow_nlu_demo_many_thresholds(session, samples_extractor_config, samples_extractor,
                                       parser_factory, nlu_demo_data, default_params, req_info):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    for intent_name, threshold in zip(['how_are_you', 'taxi'], [0.8, 0.999999999999]):
        data = nlu_demo_data[intent_name]
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data,
            name=intent_name,
            trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=threshold,
            tagger_data_keys={intent_name}
        )

    f.add_intent('dont_understand', fallback_threshold=0.8, total_fallback=True)
    f.train()

    def recognized(text):
        return f.handle(samples_extractor([text])[0], session, req_info=req_info).semantic_frames[0]['intent_name']

    # Check intents
    assert recognized('нужна машина до проспекта буденного шесть на полпятого вечера') == 'dont_understand'
    assert recognized('ты норм?') == 'how_are_you'


def test_flow_nlu_markov_transition_model(session, samples_extractor_config, samples_extractor,
                                          parser_factory, nlu_demo_data, default_params, req_info):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    threshold = 0.75
    name_to_intent = {}
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data, name=intent_name, trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=threshold,
            tagger_data_keys={intent_name}
        )
        name_to_intent[intent_name] = Intent(intent_name)

    f.add_intent('dont_understand', fallback_threshold=threshold, total_fallback=True)

    tm = MarkovTransitionModel()
    tm.add_transition(None, 'greeting')
    tm.add_transition('greeting', 'alarm')
    tm.add_transition('greeting', 'taxi')
    tm.add_transition('greeting', 'goodbye')
    tm.add_transition('alarm', 'thanks')
    tm.add_transition('alarm', 'taxi')
    tm.add_transition('taxi', 'taxi')
    tm.add_transition('taxi', 'alarm')
    tm.add_transition('taxi', 'thanks')
    tm.add_transition('thanks', 'goodbye')
    tm.build()
    f.set_transition_model(tm)

    f.train()

    def recognized(text):
        return f.handle(samples_extractor([text])[0], session, req_info).semantic_frames[0]['intent_name']
    not_recognized = 'dont_understand'

    session.clear()

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == 'greeting'
    assert recognized('машину на улицу Льва Толстого') == 'greeting'
    assert recognized('спасибо') == 'greeting'
    assert recognized('пока') == 'greeting'

    session.change_intent(name_to_intent['greeting'])

    assert recognized('привет') == not_recognized
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого') == 'taxi'
    assert recognized('спасибо') == not_recognized
    assert recognized('пока') == 'goodbye'

    session.change_intent(name_to_intent['taxi'])

    assert recognized('привет') == not_recognized
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого') == 'taxi'
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == not_recognized

    session.change_intent(name_to_intent['thanks'])

    assert recognized('привет') == 'goodbye'
    assert recognized('будильник на 7 утра') == 'goodbye'
    assert recognized('машину на улицу Льва Толстого') == 'goodbye'
    assert recognized('спасибо') == 'goodbye'
    assert recognized('пока') == 'goodbye'


def test_transition_model_from_file(session, samples_extractor_config, samples_extractor, parser_factory,
                                    nlu_demo_data, default_params, req_info):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    name_to_intent = {}
    threshold = 0.75
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data, name=intent_name, trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=threshold,
            tagger_data_keys={intent_name},
        )
        name_to_intent[intent_name] = Intent(intent_name)

    dont_understand = Intent('dont_understand')
    name_to_intent[dont_understand.name] = dont_understand
    f.add_intent(dont_understand.name, fallback_threshold=threshold, total_fallback=True)

    # Make sure loading fails for a model without all probabilities specified
    with pytest.raises(ValueError):
        create_transition_model(
            intents=name_to_intent.values(), model_name='from_file',
            filename='vins_core/test/test_data/transition_model_no_zeros.yaml')

    # Load the correct model
    tm = create_transition_model(
        intents=name_to_intent.values(), model_name='from_file',
        filename='vins_core/test/test_data/transition_model.yaml')
    f.set_transition_model(tm)

    f.train()

    def recognized(text):
        return f.handle(samples_extractor([text])[0], session, req_info=req_info).semantic_frames[0]['intent_name']
    not_recognized = 'dont_understand'

    session.clear()

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == 'greeting'
    assert recognized('машину на улицу Льва Толстого') == 'greeting'
    assert recognized('спасибо') == 'greeting'
    assert recognized('пока') == 'greeting'

    session.change_intent(name_to_intent['greeting'])

    assert recognized('привет') == not_recognized
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого') == 'taxi'
    assert recognized('спасибо') == not_recognized
    assert recognized('пока') == 'goodbye'

    session.change_intent(name_to_intent['taxi'])

    assert recognized('привет') == not_recognized
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого') == 'taxi'
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == not_recognized

    session.change_intent(name_to_intent['thanks'])

    assert recognized('привет') == 'goodbye'
    assert recognized('будильник на 7 утра') == 'goodbye'
    assert recognized('машину на улицу Льва Толстого') == 'goodbye'
    assert recognized('спасибо') == 'goodbye'
    assert recognized('пока') == 'goodbye'


def test_flow_nlu_transition_model_parent_child(session, samples_extractor_config, samples_extractor, parser_factory,
                                                nlu_demo_data, default_params, req_info):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    name_to_intent = {}
    threshold = 0.75
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data, name=intent_name, trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name=intent_name,
            fallback_threshold=threshold,
            tagger_data_keys={intent_name}
        )
        name_to_intent[intent_name] = Intent(intent_name)

    not_recognized = Intent('dont_understand')
    f.add_intent(not_recognized.name, fallback_threshold=threshold, total_fallback=True)

    name_to_intent['taxi'].parent_name = 'greeting'
    name_to_intent['alarm'].parent_name = 'greeting'
    name_to_intent['goodbye'].parent_name = 'thanks'

    f.set_transition_model(create_transition_model(name_to_intent.values()))
    f.train()

    def recognized(text):
        return f.handle(samples_extractor([text])[0], session, req_info=req_info).semantic_frames[0]['intent_name']

    session.clear()

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == not_recognized.name
    assert recognized('машину на улицу Льва Толстого 16') == not_recognized.name
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == not_recognized.name

    session.change_intent(name_to_intent['greeting'])

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого 16') == 'taxi'
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == not_recognized.name

    session.change_intent(name_to_intent['taxi'])

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == 'alarm'
    assert recognized('машину на улицу Льва Толстого 16') == 'taxi'
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == not_recognized.name

    session.change_intent(name_to_intent['thanks'])

    assert recognized('привет') == 'greeting'
    assert recognized('будильник на 7 утра') == not_recognized.name
    assert recognized('машину на улицу Льва Толстого 16') == not_recognized.name
    assert recognized('спасибо') == 'thanks'
    assert recognized('пока') == 'goodbye'


@pytest.mark.skip(reason='crossvalidation moved to tools, TODO: do the same with this test')
@pytest.mark.parametrize("intent_conditioned", (True, False))
def test_crossvalidation(mocker, nlu_obj, intent_conditioned):
    num_intents = len(nlu_obj._input_data)
    mocker.patch(
        'vins_core.nlu.token_classifier.cross_val_score',
        return_value=(0, 1, 2)
    )
    cross_val_score_returns = [
        [i] * 3 for i in xrange(-num_intents / 2, num_intents / 2)
    ] if intent_conditioned else [(-0.5, -0.5, -0.5)]
    mocker.patch(
        'vins_core.nlu.token_tagger.cross_val_score',
        side_effect=cross_val_score_returns
    )
    with mock.patch.dict(nlu_obj._utterance_tagger_config, intent_conditioned=intent_conditioned):
        cvinfo = nlu_obj.crossvalidation()
        assert cvinfo['classifier'] == {'intent_classifier_0': (1, 1)}
        assert cvinfo['tagger'] == (-0.5, 0)


def combine_classifiers_config():
    config = {
        "model": "combine_scores",
        "name": "combine_scores",
        "params": {
            "method": "multiply",
            "classifiers": []
        }
    }
    return config


@pytest.fixture(name='combine_classifiers_config')
def combine_classifiers_config_fixture():
    return combine_classifiers_config()


def data_lookup_classifier_config():
    config = {
        "model": "data_lookup",
        "name": "data_lookup",
        "params": {
            "intent_texts": {}
        }
    }
    return config


@pytest.fixture(name='data_lookup_classifier_config')
def data_lookup_classifier_config_fixture():
    return data_lookup_classifier_config()


@pytest.fixture
def dummy_classifier_config():
    config = {
        "model": "maxent",
        "name": "dummy",
        "l2reg": 1e-2,
        "features": ["word"]
    }
    return config


@pytest.mark.parametrize("config", [
    data_lookup_classifier_config(),
    combine_classifiers_config()
])
def test_classifiers_creation(dummy_classifier_config, config):
    nlu = FlowNLU(
        [config],
        [dummy_classifier_config],
        None,
        fst_parser_factory=FstParserFactory('', {}),
        fst_parsers=[]
    )
    nlu.load()
    assert nlu.has_classifier(config["name"])


def test_combine_classifier_creation(
        combine_classifiers_config, data_lookup_classifier_config, dummy_classifier_config
        ):
    combine_classifiers_config["params"]["classifiers"].append(data_lookup_classifier_config)
    nlu = FlowNLU(
        [combine_classifiers_config],
        [dummy_classifier_config],
        None,
        fst_parser_factory=FstParserFactory('', {}),
        fst_parsers=[]
    )
    nlu.load()
    assert nlu.has_classifier(data_lookup_classifier_config["name"])
    assert nlu.has_classifier(combine_classifiers_config["name"])


@pytest.mark.parametrize("cfg, classifiers_list, input, result", [
    ({'c1': {'1': ['aaa1'], '2': ['bbb1']},
      'c2': {'2': ['aaa2'], '3': ['bbb2']}},
     ['c1', 'c2'], 'aaa1', {'1': 0, '2': 0, '3': 0}),
    ({'c1': {'1': ['aaa1'], '2': ['bbb1']},
      'c2': {'2': ['aaa2'], '3': ['bbb2']}},
     ['c1'], 'aaa1', {'1': 1, '2': 0}),
    ({'c1': {'1': ['aaa1'], '2': ['bbb1']},
      'c2': {'2': ['aaa1'], '3': ['bbb2']}},
     ['c1', 'c2'], 'aaa1', {'1': 0, '2': 0, '3': 0}),
    ({'c1': {'1': ['aaa1'], '2': ['bbb1']},
      'c2': {'1': ['aaa1'], '3': ['bbb2']}},
     ['c1', 'c2'], 'aaa1', {'1': 1, '2': 0, '3': 0}),
])
def test_compute_likelihoods(cfg, classifiers_list, input, result, combine_classifiers_config,
                             data_lookup_classifier_config, dummy_classifier_config):
    intents = set()
    for clf_name in classifiers_list:
        classifier_config = copy.deepcopy(data_lookup_classifier_config)
        classifier_config["name"] = clf_name
        classifier_config["params"]["intent_texts"] = cfg[clf_name]
        combine_classifiers_config["params"]["classifiers"].append(classifier_config)
        intents.update((intent for intent in cfg[clf_name].keys()))
    nlu = FlowNLU(
        [combine_classifiers_config],
        [dummy_classifier_config],
        None,
        fst_parser_factory=FstParserFactory('', {}),
        fst_parsers=[]
    )
    for intent in intents:
        nlu.add_intent(intent)
    nlu.load()
    feature = SampleFeatures(Sample.from_string(input))
    combine_scores_classifier = nlu.get_classifier("combine_scores")
    assert nlu._intent_classifiers_cascade.compute_likelihoods(
        combine_scores_classifier, feature, req_info=None
    ) == result


def test_train_with_validation(nlu):
    results = nlu.train(validation=0.5)
    assert 'taggers_validation_result' in results
    assert len(results['taggers_validation_result']) > 0


def test_tagger_train_chunks(default_params, parser_factory, normalize_sample_processor):
    class RaiseExceptionSampleProcessor(BaseSampleProcessor):
        NAME = 'raise_exception'

        @property
        def is_normalizing(self):
            return True

        def _process(self, sample, *args, **kwargs):
            if sample.text == 'fst exception':
                raise FstNormalizerError()
            if sample.text == 'empty exception':
                raise NormalizerEmptyResultError()
            return sample

    pipeline = [normalize_sample_processor, RaiseExceptionSampleProcessor()]
    se = SamplesExtractor(pipeline)

    intents = {
        'unk': [
            'asfadf'
        ],
        'test': [
            'тестируем "abc"(slot)',
            'тестируем "def"(slot)',
        ]
    }
    f = nlu(intents, None, parser_factory, default_params)
    f._samples_extractor = se
    f.add_input_data('test', map(NluSourceItem, ['ничего', 'fst exception', 'empty exception', 'особенного']))
    f.add_input_data('test', [NluSourceItem('не должно быть в обучении тэггера', can_use_to_train_tagger=False)])
    f.train(classifiers=['intent_classifier_0', 'intent_classifier_1'])
    # trained classifier without errors

    # we need to pass some non-trainable classifier for "test" intent
    # so that `can_use_to_train_tagger=False` data chunks were skipped during sample extraction o_O
    f.train(classifiers=['non_trainnable_classifier'])
    # trained tagger without errors


def test_flow_nlu_new_parameters_on_train(samples_extractor_config, parser_factory, nlu_demo_data, default_params):
    f = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    f.load()
    for intent_name, data in nlu_demo_data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(
            data,
            name=intent_name,
            trainable_classifiers=['intent_classifier_0', 'intent_classifier_1']
        )
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(
            intent_name,
            fallback_threshold=0.8,
            tagger_data_keys={intent_name}
        )

    f.add_intent('dont_understand', fallback_threshold=0.8, total_fallback=True)

    # we train the model first with default parameters, to obtain the public property final_estimator
    f.train()
    assert f.get_classifier('intent_classifier_0').final_estimator.C == 1 / 1e-2
    assert f.get_classifier('intent_classifier_1').final_estimator.C == 1 / 1e-2
    assert len(f.classifiers_cascade) == 1

    # now retrain and see whether the new kwarg reaches its target estimator
    f.train(l2reg=4e-2)
    assert f.get_classifier('intent_classifier_0').final_estimator.C == 1 / 4e-2
    assert f.get_classifier('intent_classifier_1').final_estimator.C == 1 / 4e-2
    # additionally, check that the cascade did not increase
    assert len(f.classifiers_cascade) == 1


def test_nlu_predict_intents(mocker, samples_extractor_config, parser_factory, default_params, req_info):
    assert _DEFAULT_FALLBACK_THRESHOLD == 0.8
    nlu = FlowNLU(
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **default_params
    )
    nlu.load()
    nlu.add_classifier(classifier=Classifier())
    internal_intent_return_list = {
        'intent_1': 0.90,
        'intent_2': 0.60,
        'intent_3': 0.75,
        'intent_4': 0.69,
        'intent_5': 0.70,
        'fallback_intent': 0.65
    }
    for intent_name, fallback_threshold, fallback in [
        ('intent_1', 0.99, False),
        ('intent_2', 0.50, False),
        ('intent_3', None, False),
        ('intent_4', 0.60, False),
        ('intent_5', 0.60, False),
        ('fallback_intent', 0.10, True),
    ]:
        nlu.add_intent(
            intent_name=intent_name,
            fallback_threshold=fallback_threshold,
            fallback=fallback,
            total_fallback=fallback
        )
    nlu._intent_classifiers_cascade.compute_likelihoods = mock.MagicMock(return_value=internal_intent_return_list)
    nlu._intent_classifiers_cascade.compute_posteriors = mock.MagicMock(return_value=internal_intent_return_list)
    candidates = nlu._predict_intents(feature=None, session=None, req_info=req_info)
    assert len(candidates) == 4
    assert [candidate.name for candidate in candidates] == ['intent_5', 'intent_4', 'fallback_intent',
                                                            'intent_2']
    assert [candidate.score for candidate in candidates] == [0.70, 0.69, 0.65, 0.6]
    assert [candidate.is_fallback for candidate in candidates] == [False, False, True, False]
