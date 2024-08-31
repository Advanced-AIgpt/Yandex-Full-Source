# coding: utf-8
from __future__ import unicode_literals

import pytest
import operator
import copy

from uuid import uuid4
from freezegun import freeze_time

from vins_core.common.entity import Entity
from vins_core.ner.fst_presets import PARSER_RU_BASE_PARSERS
from vins_core.common.slots_map_utils import get_slot_value
from vins_core.dm.session import Session
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.dm.frame_scoring.slot_type import SlotTypeFrameScoring, ActiveSlotFrameScoring, DisabledSlotFrameScoring
from vins_core.dm.frame_scoring.tagger_score import TaggerScoreFrameScoring
from vins_core.dm.frame_scoring.multislot_scoring import MultislotFrameScoring
from vins_core.dm.form_filler.models import Form, Slot, SlotConcatenation
from vins_core.dm.form_filler.nlu_postprocessor import (
    RescoringNluPostProcessor, TopSelectorNluPostProcessor, CutoffNluPostProcessor,
    DatetimeNowNluPostProcessor, SortNluPostProcessor, NormalizingNluPostProcessor, ContinuedSlotNluPostProcessor
)
from vins_core.nlu.flow_nlu import FlowNLU
from vins_core.utils.datetime import utcnow
from vins_core.common.slots_map_utils import tags_to_slots


@pytest.fixture(scope='function')
def session():
    return Session(app_id='123', uuid=uuid4())


@pytest.fixture(scope='function')
def forms_map():
    f = dict()
    f['taxi'] = Form.from_dict({
        'name': 'taxi',
        'slots': [
            {
                'slot': 'location_from',
                'types': ['geo'],
                'optional': False,
            },
            {
                'slot': 'location_to',
                'types': ['geo'],
                'optional': False
            },
            {
                'slot': 'when',
                'types': ['datetime'],
                'optional': False
            },
        ]
    })
    f['alarm'] = Form.from_dict({
        'name': 'alarm',
        'slots': [
            {
                'slot': 'when',
                'types': ['datetime'],
                'optional': False
            },
        ]
    })
    f['call'] = Form.from_dict({
        'name': 'call',
        'slots': [
            {
                "slot": "recipient",
                "types": ["known_phones", "fio", "string"],
                "optional": False,
                "normalize_to": ["nomn"],
            }
        ]
    })
    f['search'] = Form.from_dict({
        'name': 'search',
        'slots': [
            {
                "slot": "query",
                "type": "string",
                "optional": False,
                "prefix_normalization": [
                    {
                        "source_case": "accs",
                        "prefix": "((.* |^)про|((найди|поищи)(| мне)))$"
                    },
                    {
                        "source_case": "loct",
                        "prefix": "(.* |^)о(б|)$"
                    }
                ]
            }
        ]
    })
    return f


def get_nlu(samples_extractor_config, parser_factory, **params):
    data = {
        'alarm': [
            'будильник на "8:30"(when)',
            'поставь будильник на "6 утра"(when)',
            'разбуди меня "завтра в без 15 4"(when) обязательно',
            'разбуди меня в "9 вечера"(when)',
            'разбудить',
            'будильник на "полпятого вечера"(when)'
        ],
        'taxi': [
            'заберите "иванова ивана"(client) из "иваново"(location_from)',
            'заберите "петрова петра"(client) из "набережных челнов"(location_from)',
            "едем из 'набережных челнов'(location_from)",
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машина до "рокоссовского дом 16"(location_to)',
            'машину от "тверской улицы"(location_from) в "без 15 6 вечера"(when) не дороже "500 рублей"(price)',
            '"город Москва улица Тимура Фрунзе дом 16/1"(location_from)',
            'нужно такси на "ленинградский проспект дом 9 корпус 1"(location_from) на "6 часов утра"(when)',
            'нужна машина на "улицу ленина 12"(location_from) на "6 часов утра"(when)',
            'такси от "зеленого проспекта"(location_from) до "чистопрудного бульвара"(location_to)',
            'из "москвы"(location_from) до "воронежа"(location_to) "завтра"(when)',
            "хочу заказать такси на '8 утра'(when)",
            "закажи мне такси на 'улицы маршала неделина дом 6'(location_from)",
            "мне нужно такси в 'москву'(location_to)",
            'нужна машина на "полпятого вечера"(when)'

            "закажи мне такси",
            "мне нужно такси",
            "закажи такси",
            "закажи мне такси",
            "мне нужна тачка",
            "отвезите меня",
            "мне нужно ехать",
            "как можно быстрее закажите такси"
        ],
        'call': [
            'позвони "ивану иванову"(recipient)',
            'позвони "ивану петрову"(recipient)',
            'позвони "марии федоровой"(recipient)',
            'набери "марину"(recipient)',
            'вызови "бэтмена"(recipient)',
            'позвони "маме"(recipient)',
            'позвони "бабушке"(recipient)',
            'позвони "сантехнику"(recipient)',
            'позвони "директору"(recipient)'
        ],
        'search': [
            'расскажи про "гагарина"(query)',
            'расскажи про "королева"(query)',
            'найди "королева"(query)',
            'поищи "королева"(query)',
            'а про "королева"(query)',
            'расскажи о "гагарине"(query)',
            'расскажи "гагарина"(query)',
            'расскажипро "гагарина"(query)',
        ]
    }
    f = FlowNLU(
        feature_extractors=[
            {'type': 'ngrams', 'id': 'word', 'n': 1},
            {'type': 'ngrams', 'id': 'bigram', 'n': 2},
            {'type': 'ner', 'id': 'ner'},
            {'type': 'postag', 'id': 'postag'},
            {'type': 'lemma', 'id': 'lemma'}
        ],
        intent_classifiers=[{
            'name': 'intent_classifier_0',
            'model': 'maxent',
            'features': ['word', 'bigram', 'ner', 'postag', 'lemma']
        }],
        utterance_tagger={
            'model': 'crf',
            'features': ['word', 'lemma', 'ner'],
            'params': {'intent_conditioned': True}
        },
        samples_extractor=samples_extractor_config,
        fst_parser_factory=parser_factory,
        fst_parsers=PARSER_RU_BASE_PARSERS,
        **params
    )
    f.load()
    for intent_name, data in data.iteritems():
        nlu_sources = FuzzyNLUFormat.parse_iter(data, name=intent_name, trainable_classifiers=['intent_classifier_0'])
        f.add_input_data(intent_name, nlu_sources.items)
        f.add_intent(intent_name, fallback_threshold=0.01, tagger_data_keys=[intent_name])
    f.add_intent('dont_understand', total_fallback=True)
    f.train()
    return f


class Pipeline(object):

    def __init__(self, processors):
        self._processors = processors

    def __call__(self, frames, session, **kwargs):
        out = copy.deepcopy(frames)
        for processor in self._processors:
            out = processor(out, session, **kwargs)
        return out


@pytest.fixture(scope='module')
def nlu(samples_extractor_config, parser_factory):
    return get_nlu(samples_extractor_config, parser_factory)


def test_rescoring_slot_type(nlu, samples_extractor, session, forms_map):
    frames = nlu.handle(
        samples_extractor(['нужна машина до проспекта буденного шесть на полпятого вечера'])[0],
        session, max_intents=10
    ).semantic_frames
    rescoring = RescoringNluPostProcessor([SlotTypeFrameScoring()])

    # selected form is identical to session
    session.change_form(forms_map['taxi'])
    out_frames = filter(operator.itemgetter('confidence'), rescoring(frames, session, forms_map=forms_map))
    assert len(out_frames) < len(frames)
    assert TopSelectorNluPostProcessor()(out_frames, session)[0]['intent_name'] == 'taxi'
    for frame in out_frames:
        for slot in frame['slots'].itervalues():
            assert any(e.type in ('GEO', 'DATETIME') for slot_item in slot for e in slot_item['entities'])

    # selected form is different from session
    session.change_form(forms_map['alarm'])
    out_frames = filter(operator.itemgetter('confidence'), rescoring(frames, session, forms_map=forms_map))
    assert len(out_frames) < len(frames)
    for frame in out_frames:
        for slot in frame['slots'].itervalues():
            assert any(e.type in ('GEO', 'DATETIME') for slot_item in slot for e in slot_item['entities'])


@pytest.mark.parametrize('matching_type, location_to_slot', [
    ('exact', {'start': 3, 'end': 6, 'substr': 'проспекта буденного шесть'}),
    ('inside', {'start': 2, 'end': 6, 'substr': 'до проспекта буденного шесть'}),
    ('overlap', {'start': 4, 'end': 6, 'substr': 'буденного шесть'})
])
def test_rescoring_slot_type_matching(nlu,
                                      samples_extractor,
                                      session,
                                      forms_map,
                                      matching_type,
                                      location_to_slot):
    frames = nlu.handle(
        samples_extractor(['нужна машина до проспекта буденного шесть на полпятого вечера'])[0],
        session, max_intents=10
    ).semantic_frames
    rescoring = RescoringNluPostProcessor([SlotTypeFrameScoring()])

    # Patching slots in frame returned by nlu.
    entities = [Entity(start=3, end=6, value={'street': 'пр-кт. Буденного', 'house': 6}, type='GEO')]
    patched_frame = copy.deepcopy(frames[0])
    patched_frame['slots']['location_to'][0] = location_to_slot
    patched_frame['slots']['location_to'][0]['entities'] = entities

    # Patching 'location_to' slot in forms_map['taxi'] to have custom matching_type.
    patched_forms_map = copy.deepcopy(forms_map)
    patched_forms_map['taxi'].get_slot_by_name('location_to').matching_type = matching_type

    out_frames = rescoring([patched_frame], session, forms_map=patched_forms_map)

    # Assert that SlotTypeFrameScoring has not dropped frame. (so matched entity with slot).
    assert len(out_frames) == 1


def test_rescoring_slot_type_active(nlu, samples_extractor, session, forms_map):
    frames = nlu.handle(
        samples_extractor(['нужна машина проспект буденного шесть на полпятого вечера'])[0],
        session
    ).semantic_frames
    rescoring = Pipeline([
        RescoringNluPostProcessor([
            SlotTypeFrameScoring(),
            ActiveSlotFrameScoring(boosting_score=99)
        ]),
        CutoffNluPostProcessor(cutoff=0)
    ])

    active_slot = 'location_from'
    form = copy.deepcopy(forms_map['taxi'])

    def set_active_slot(active):
        for slot in form.slots:
            if slot.name == active_slot:
                slot.active = active

    set_active_slot(True)
    session.change_form(form)

    out_frames = rescoring(copy.deepcopy(frames), session, forms_map=forms_map)
    assert len(out_frames) < len(frames)
    top_frame = TopSelectorNluPostProcessor()(out_frames, session)[0]
    assert top_frame['intent_name'] == 'taxi'
    assert active_slot in top_frame['slots']

    # test fallback behaviour when no active slots found: should be identical as no active slot rescorer is specified
    set_active_slot(True)
    no_active_slot_frames = filter(lambda f: active_slot not in f['slots'], copy.deepcopy(frames))
    active_on = rescoring(no_active_slot_frames, session, forms_map=forms_map)
    assert len(active_on) < len(frames)

    set_active_slot(False)
    session.change_form(form)
    rescoring2 = Pipeline([
        RescoringNluPostProcessor([
            SlotTypeFrameScoring(),
        ]),
        CutoffNluPostProcessor(cutoff=0)
    ])
    active_off = rescoring2(no_active_slot_frames, session, forms_map=forms_map)
    assert active_on == active_off


@pytest.mark.parametrize("disabled_slots,expected_frame_slots", [
    ([], ["when"]),
    (["when"], []),
])
def test_rescoring_slot_type_disabled(disabled_slots, expected_frame_slots,
                                      nlu, samples_extractor, session, forms_map):
    frames = nlu.handle(
        samples_extractor(['нужна машина на полпятого вечера'])[0],
        session
    ).semantic_frames
    rescorer = Pipeline([
        RescoringNluPostProcessor([
            DisabledSlotFrameScoring(0)
        ]),
        CutoffNluPostProcessor(),
        TopSelectorNluPostProcessor()
    ])
    form = copy.deepcopy(forms_map['taxi'])
    for slot in form.slots:
        if slot.name in disabled_slots:
            slot.disabled = True
    session.change_form(form)

    frame = rescorer(frames, session)
    found_slots = frame[0]['slots'].keys()
    assert set(found_slots) == set(expected_frame_slots)


def test_rescoring_slot_type_default_active_score(nlu, samples_extractor, session, forms_map):
    frames = nlu.handle(
        samples_extractor(['нужна машина до проспекта буденного шесть на полпятого вечера'])[0],
        session
    ).semantic_frames

    rescoring = RescoringNluPostProcessor([ActiveSlotFrameScoring()], normalize=False)

    active_slot = 'location_from'
    form = copy.deepcopy(forms_map['taxi'])

    def set_active_slot(active):
        for slot in form.slots:
            if slot.name == active_slot:
                slot.active = active

    # active slot is presented
    set_active_slot(True)
    session.change_form(form)
    assert rescoring(frames, session, forms_map=forms_map) == frames

    # active slot is presented ..
    set_active_slot(True)
    session.change_form(form)
    # ..but frames are active slot-less
    filter_frames = filter(lambda f: active_slot not in f['slots'], copy.deepcopy(frames))
    assert rescoring(filter_frames, session, forms_map=forms_map) == filter_frames


def test_rescoring_tagger_score(nlu, samples_extractor, session):
    frames = nlu.handle(
        samples_extractor(['нужна машина до проспекта буденного шесть на полпятого вечера'])[0],
        session, max_intents=10
    ).semantic_frames
    rescoring = RescoringNluPostProcessor([TaggerScoreFrameScoring(normalize=True, multiply_confidence=True)])
    out_frames = SortNluPostProcessor()(rescoring(frames, session), session)
    when = {
        'substr': 'полпятого вечера',
        'end': 9,
        'entities': [
            Entity(start=7, end=9, type='DATETIME', value={'hours': 16, 'minutes': 30}),
            Entity(start=7, end=9, type='TIME', value={'hours': 4, 'minutes': 30, 'period': 'pm'}),
        ],
        'start': 7,
        'is_continuation': False
    }
    # check that first suggestion is relevant
    assert out_frames[0]['intent_name'] == 'taxi'
    assert out_frames[0]['slots']['when'][0] == when
    # also check that other scenarios get rescored too
    assert set(frame['intent_name'] for frame in out_frames) == {'taxi', 'alarm'}


@freeze_time("2017-01-11 17:30")
@pytest.mark.parametrize("utt,datetime_types,output", [
    ('разбуди меня через полчаса', ['datetime'],
     {'seconds': 0, 'months': 1, 'days': 11, 'years': 2017, 'hours': 18, 'minutes': 0}),
    ('разбуди меня через полчаса', ['datetime_raw'],
     {'minutes': 30, 'minutes_relative': True}),
    ('разбуди меня через четыре миллиона лет', ['datetime'],
     {'seconds': 59, 'months': 12, 'days': 31, 'years': 9999, 'hours': 23, 'minutes': 59}),
    ('разбуди меня четыре миллиона лет назад', ['datetime'],
     {'seconds': 0, 'months': 1, 'days': 1, 'years': 1, 'hours': 0, 'minutes': 0}),
    ('разбуди меня через четыре миллиарда недель', ['datetime'],
     {'seconds': 59, 'months': 12, 'days': 31, 'years': 9999, 'hours': 23, 'minutes': 59, 'weeks': 4000000000}),
    ('разбуди меня через четыре миллиарда месяцев', ['datetime'],
     {'seconds': 59, 'months': 12, 'days': 31, 'years': 9999, 'hours': 23, 'minutes': 59}),
    ('разбуди меня через сто миллиардов минут', ['datetime'],
     {'seconds': 59, 'months': 12, 'days': 31, 'years': 9999, 'hours': 23, 'minutes': 59}),
    ('разбуди меня через сто миллиардов секунд', ['datetime'],
     {'seconds': 40, 'months': 11, 'days': 27, 'years': 5185, 'hours': 3, 'minutes': 16}),
    pytest.param('разбуди меня через полчаса', 'undefined_type', {}, marks=pytest.mark.xfail()),
])
def test_apply_datetime_now(utt, datetime_types, output, nlu, samples_extractor, session, forms_map):
    forms_map_ = copy.deepcopy(forms_map)
    forms_map_['alarm'].get_slot_by_name('when').types = datetime_types
    frames = nlu.handle(
        samples_extractor([utt])[0],
        session
    ).semantic_frames
    post_process = Pipeline([
        RescoringNluPostProcessor([SlotTypeFrameScoring(), TaggerScoreFrameScoring()]),
        DatetimeNowNluPostProcessor(),
        TopSelectorNluPostProcessor()
    ])
    frame = post_process(frames, session, forms_map=forms_map_, client_time=utcnow())[0]
    assert get_slot_value(frame['slots']['when'][0], 'DATETIME') == output


@pytest.mark.parametrize("utt,output", [
    ('ивану', 'иван'),
    ('ивану иванову', 'иван иванов'),
    ('марии', 'мария'),
    ('бабушке', 'бабушка'),
    ('маме', 'мама'),
])
def test_apply_normalization(utt, output, nlu, samples_extractor, session, forms_map):
    forms_map_ = copy.deepcopy(forms_map)
    sample = samples_extractor([utt])[0]
    frames = nlu.handle(
        sample,
        session
    ).semantic_frames
    post_process = Pipeline([
        RescoringNluPostProcessor([SlotTypeFrameScoring(), TaggerScoreFrameScoring()]),
        NormalizingNluPostProcessor(),
        TopSelectorNluPostProcessor()
    ])
    frame = post_process(frames, session, sample=sample, forms_map=forms_map_, client_time=utcnow())[0]
    assert frame['slots']['recipient'][0]['substr'] == output


@pytest.mark.parametrize("utt,output", [
    ('расскажи про гагарина', 'гагарин'),
    ('расскажи про королёва', 'королёв'),
    ('найди королёва', 'королёв'),
    ('поищи королёва', 'королёв'),
    ('а про королева', 'королёв'),
    ('расскажи о гагарине', 'гагарин'),
    ('расскажи гагарина', 'гагарина'),
    ('расскажипро гагарина', 'гагарина')
])
def test_apply_prefixwise_normalization(utt, output, nlu, samples_extractor, session, forms_map):
    forms_map_ = copy.deepcopy(forms_map)
    sample = samples_extractor([utt])[0]
    frames = nlu.handle(
        sample,
        session
    ).semantic_frames
    post_process = Pipeline([
        RescoringNluPostProcessor([SlotTypeFrameScoring(), TaggerScoreFrameScoring()]),
        NormalizingNluPostProcessor(),
        TopSelectorNluPostProcessor()
    ])
    frame = post_process(frames, session, sample=sample, forms_map=forms_map_, client_time=utcnow())[0]
    assert frame['slots']['query'][0]['substr'] == output


@pytest.mark.parametrize("concatenation,expected_product_values", [
    (SlotConcatenation.allow, ['iphone 7', 'samsung galaxy']),
    (SlotConcatenation.force, ['iphone 7 samsung galaxy']),
    (SlotConcatenation.forbid, ['iphone 7', 'samsung', 'galaxy']),
])
def test_multiple_slots_merge(concatenation, expected_product_values, samples_extractor):
    utt = "хочу купить 'iphone 7'(product) и 'samsung'(product) еще 'galaxy'(+product) на 'беру точка ру'(shop)"
    item = FuzzyNLUFormat.parse_one(utt)
    sample = samples_extractor([item])[0]
    slots, _ = tags_to_slots(sample.tokens, sample.tags)
    semantic_frames = [{'intent_name': 'buy', 'slots': slots}]
    slot_config = [
        Slot('product', ['string'], concatenation=concatenation),
        Slot('shop', ['string'])
    ]
    forms = {'buy': Form('buy', slots=slot_config)}
    processor = ContinuedSlotNluPostProcessor()
    result = processor(semantic_frames, None, forms_map=forms)
    product_slots = result[0]['slots']['product']
    assert [slot['substr'] for slot in product_slots] == expected_product_values


def test_exclude_multiple_slots(nlu, samples_extractor, session, forms_map):
    utt = 'заберите петрова петра из набережных челнов заберите петрова петра из набережных челнов'
    frames = nlu.handle(
        samples_extractor([utt])[0],
        session, max_intents=10
    ).semantic_frames
    rescoring = RescoringNluPostProcessor([MultislotFrameScoring(forms_map=forms_map)])
    cutoff = CutoffNluPostProcessor(cutoff=0, dont_kill_all_frames=True)

    post_processor_args = {
        'forms_map': forms_map
    }
    frames = rescoring(frames, session, **post_processor_args)
    frames = cutoff(frames, session, **post_processor_args)
    result = frames[0]

    # in the winning case there would be 0 or 1 occurrence of each tag
    for slot, slot_value in result['slots'].iteritems():
        assert not isinstance(slot_value, list), "{} has {} occurrences, <=1 expected".format(slot, len(slot_value))


@pytest.mark.parametrize("frames, allow_multiple_action, expected_slots", [
    ([{'intent_name': 'stock_market', 'slots': {'action': [{'value': 'sell'}]}}],
     False,
     {'action': [{'value': 'sell'}]}),
    ([{'intent_name': 'stock_market', 'slots': {'action': [{'value': 'sell'}, {'value': 'buy'}]}}],
     False,
     {}),
    ([{'intent_name': 'stock_market', 'slots': {'action': [{'value': 'sell'}, {'value': 'buy'}]}}],
     True,
     {'action': [{'value': 'sell'}, {'value': 'buy'}]})

])
def test_manage_multiple_slots(frames, allow_multiple_action, expected_slots):
    stock_forms_map = {
        'stock_market': Form.from_dict(
            {
                'name': 'stock_market',
                'slots': [
                    {
                        'name': 'action',
                        'type': 'string',
                        'optional': True,
                        'allow_multiple': allow_multiple_action
                    }
                ]
            }
        )
    }
    processors = [
        RescoringNluPostProcessor([MultislotFrameScoring(forms_map=stock_forms_map)]),
        CutoffNluPostProcessor(cutoff=0, dont_kill_all_frames=True),
        SortNluPostProcessor()
    ]
    for processor in processors:
        frames = processor(frames, session=None, forms_map=stock_forms_map)
    result = frames[0]
    assert result['slots'] == expected_slots
