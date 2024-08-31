# coding: utf-8
from __future__ import unicode_literals

from collections import Counter

import pytest

import vins_core

from vins_core.common.utterance import Utterance
from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat, NluSourceItem
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.nlu.samples_extractor import SamplesExtractor, SamplesExtractorError
from vins_core.nlu.sample_processors.normalize import FstNormalizerError


class TestSampleProcessorError(Exception):
    pass


class TestFailingSampleProcessor(BaseSampleProcessor):
    NAME = 'failing'

    @property
    def is_normalizing(self):
        return False

    def _process(self, sample, session, is_inference, *args, **kwargs):
        raise TestSampleProcessorError


@pytest.fixture(scope='module')
def samples_extractor_nonorm():
    return SamplesExtractor()


def test_empty_samples_extractor_does_not_corrupt_sample(samples_extractor_nonorm):
    sex = samples_extractor_nonorm

    none_processed_sample = sex([None])[0]
    assert none_processed_sample == Sample.from_none()

    test_utt = 'тестируемся тестируемся тестируемся'
    processed_sample = sex([test_utt])[0]
    assert processed_sample == Sample.from_string(test_utt)


def test_samples_extractor_returns_empty_list(samples_extractor_nonorm):
    assert samples_extractor_nonorm([]) == []


def test_samples_extractor_returns_samples_when_flag_is_passed(samples_extractor):
    utts = [
        '"улица Льва Толстого 16/1"(where)',
        'будильник на "семь утра"(when)!',
        'погода в "москве"(where) "двадцать пятого мая"(when)',
        '\n- "Хайвей 1 Гб" (абонентская плата 200.00 ₽/мес),',
        '\n- мне нужен \'"Хайвей 1 Гб" (абонентская плата 200.00 ₽/мес),\'(tariff) на телефон',
    ]
    nlu_source_items = FuzzyNLUFormat.parse_iter(utts).items

    samples = samples_extractor(nlu_source_items)
    assert all([isinstance(s, Sample) for s in samples])


def test_samples_extractor(samples_extractor):
    utts = [
        '"улица Льва Толстого 16/1"(where)',
        'будильник на "семь утра"(when)!',
        'погода в "москве"(where) "двадцать пятого мая"(when)',
        '\n- "Хайвей 1 Гб" (абонентская плата 200.00 ₽/мес),',
        '\n- мне нужен \'"Хайвей 1 Гб" (абонентская плата 200.00 ₽/мес),\'(tariff) на телефон',
    ]
    nlu_source_items = FuzzyNLUFormat.parse_iter(utts).items
    samples = samples_extractor(nlu_source_items)

    assert len(samples) == len(utts)
    assert samples[0].tokens == ['улица', 'льва', 'толстого', '16', '/', '1']
    assert samples[0].tags == ['B-where', 'I-where', 'I-where', 'I-where', 'I-where', 'I-where']
    assert samples[1].tokens == ['будильник', 'на', '7', 'утра']
    assert samples[1].tags == ['O', 'O', 'B-when', 'I-when']
    assert samples[2].tokens == ['погода', 'в', 'москве', '25', 'мая']
    assert samples[2].tags == ['O', 'O', 'B-where', 'B-when', 'I-when']
    assert samples[3].tokens == ['-', 'хайвей', '1', 'гб', 'абонентская', 'плата', '200.00', '₽', 'мес']
    assert samples[3].tags == ['O', 'O', 'O', 'O', 'O', 'O', 'O', 'O', 'O']
    assert samples[4].tokens == ['-', 'мне', 'нужен', 'хайвей', '1', 'гб',
                                 'абонентская', 'плата', '200.00', '₽', 'мес', 'на', 'телефон']
    assert samples[4].tags == ['O', 'O', 'O', 'B-tariff', 'I-tariff', 'I-tariff', 'I-tariff',
                               'I-tariff', 'I-tariff', 'I-tariff', 'I-tariff', 'O', 'O']


def test_continuation_slots_extractor(samples_extractor):
    utts = [
        'хочу купить "айфон 7"(product) и "samsung"(product) еще "galaxy"(+product)',
    ]
    nlu_source_items = FuzzyNLUFormat.parse_iter(utts).items
    assert [slot.is_continuation for slot in nlu_source_items[0].slots] == [False, False, True]
    samples = samples_extractor(nlu_source_items)
    assert samples[0].tags == ['O', 'O', 'B-product', 'I-product', 'O', 'B-product', 'O', 'I-product']


def test_samples_misaligned_normalization(samples_extractor):
    utts = [
        'я передумал, - подайте машину на "тверскую, 1"(location_from)',
        'нет, "я"(client) поеду   на "лубянку"(location_to) !)($    ',
        "информация по запросу 'два отца и два сына 2'(query)",
        "'çıpla     baŞan  -- fgraflar'(query)",
        '  встретимся "тет-а-тет"(tet-a-tet)',
        '  встретимся "тет-а - тет"(tet-a-tet)',
        'будильник на "десять : тридцать пять утра "(alarm)',
        "'двадцать пять'(num1) 'шестнадцать'(num2)",
        '"двадцать пятого мая 2016-го"(date) в "10 часов и тридцать пять минут"(time)',
        '"двадцать пятому маю"(date) "10 часов"(time)',
        "на Интернет '000000000 555'(num-1), на телефон '0000000000'(num-2)"
    ]
    sources = FuzzyNLUFormat().parse_iter(utts, None)
    samples = samples_extractor(sources.items)

    assert ' '.join(samples[0].tokens) == 'я передумал - подайте машину на тверскую 1'
    assert ' '.join(samples[0].tags) == 'O O O O O O B-location_from I-location_from'
    assert ' '.join(samples[1].tokens) == 'нет я поеду на лубянку $'
    assert ' '.join(samples[1].tags) == 'O B-client O O B-location_to O'
    assert ' '.join(samples[2].tokens) == 'информация по запросу 2 отца и 2 сына 2'
    assert ' '.join(samples[2].tags) == 'O O O B-query I-query I-query I-query I-query I-query'
    assert ' '.join(samples[3].tokens) == 'çıpla baŞan fgraflar'.lower()
    assert ' '.join(samples[3].tags) == 'B-query I-query I-query'
    assert ' '.join(samples[4].tokens) == 'встретимся тет-а-тет'
    assert ' '.join(samples[4].tags) == 'O B-tet-a-tet'
    assert ' '.join(samples[5].tokens) == 'встретимся тет-а - тет'
    assert ' '.join(samples[5].tags) == 'O B-tet-a-tet I-tet-a-tet I-tet-a-tet'
    assert ' '.join(samples[6].tokens) == 'будильник на 10 35 утра'
    assert ' '.join(samples[6].tags) == 'O O B-alarm I-alarm I-alarm'
    assert ' '.join(samples[7].tokens) == '25 16'
    assert ' '.join(samples[7].tags) == 'B-num1 B-num2'
    assert ' '.join(samples[8].tokens) == '25 мая 2016 го в 10 часов и 35 минут'
    assert ' '.join(samples[8].tags) == 'B-date I-date I-date I-date O B-time I-time I-time I-time I-time'
    assert ' '.join(samples[9].tokens) == '25 маю 10 часов'
    assert ' '.join(samples[9].tags) == 'B-date I-date B-time I-time'
    assert ' '.join(samples[10].tokens) == (
        'на интернет 00000-00-00 555 на телефон (000)000-00-00'
    )
    assert ' '.join(samples[10].tags) == 'O O B-num-1 I-num-1 O O B-num-2'


def test_samples_misaligned_normalization_error(samples_extractor):
    utts = [
        "'сто двадцать'(num1) пять"
    ]
    sources = FuzzyNLUFormat().parse_iter(utts, None)
    for item in sources.items:
        with pytest.raises(FstNormalizerError):
            samples_extractor([item], ignore_exceptions=())


def test_samples_misaligned_multislot(samples_extractor):
    sample = samples_extractor(
        FuzzyNLUFormat().parse_iter(
            ['"двадцать пятое мая"(date) "двадцать шестое мая"(date)'], None
        ).items
    )[0]
    assert sample.text == '25 мая 26 мая'
    assert ' '.join(sample.tags) == 'B-date I-date B-date I-date'


@pytest.mark.xfail(reason='fst normalizer error')
def test_samples_misaligned_fst_normalizer_error(samples_extractor):
    sample = samples_extractor([
        NluSourceItem('найти в поиске новые игры на xbox 360 2016 года')
    ])[0]
    assert sample.text == 'найти в поиске новые игры на xbox 360 2016 года'


def test_samples_extractor_change_normalizer(samples_extractor, samples_extractor_nonorm):
    item = NluSourceItem('need an inexpensive flight, from baltimore to san francisco!')
    ru_out = ['need', 'an', 'inexpensive', 'flight', 'from', 'baltimore', 'to', 'san', 'francisco']
    no_out = ['need', 'an', 'inexpensive', 'flight,', 'from', 'baltimore', 'to', 'san', 'francisco!']
    assert samples_extractor([item])[0].tokens == ru_out
    assert samples_extractor_nonorm([item])[0].tokens == no_out


def test_samples_remove_comma_between_words(samples_extractor):
    samples = samples_extractor(FuzzyNLUFormat.parse_iter([
        'мир,труд,май,1,мая,1 мая,hsdf',
        'ikjdsfl,ijap,qw[po',
        'это "мир,труд,май,1,мая,1-мая,100$"(x) это',
    ]).items)
    assert samples[0].text == 'мир труд май 1 мая 1 мая hsdf'
    assert samples[1].text == 'ikjdsfl ijap qw po'
    assert ' '.join(samples[2].tokens) == 'это мир труд май 1 мая 1 мая 100 $ это'
    assert ' '.join(samples[2].tags) == 'O B-x I-x I-x I-x I-x I-x I-x I-x I-x O'


def test_samples_extractor_nlu_source_item_input(samples_extractor):
    from vins_core.dm.formats import FuzzyNLUFormat
    r = samples_extractor(FuzzyNLUFormat.parse_iter(['будильник на "семь утра"(when)!']).items)
    assert r[0].tokens == ['будильник', 'на', '7', 'утра']
    assert r[0].tags == ['O', 'O', 'B-when', 'I-when']


def test_samples_extractor_string_input(samples_extractor):
    r = samples_extractor(['будильник на "семь утра"!'])
    assert r[0].tokens == ['будильник', 'на', '7', 'утра']
    assert r[0].tags == ['O', 'O', 'O', 'O']


def test_samples_extractor_none_input(samples_extractor):
    r = samples_extractor([None])
    assert isinstance(r[0], Sample) and not r[0]


def test_samples_extractor_mixed_input(samples_extractor):
    r = samples_extractor([
        FuzzyNLUFormat.parse_iter(['будильник на "семь утра"(when)!']).items[0],
        'будильник на "семь утра"!',
        None
    ])

    assert r[0].tokens == ['будильник', 'на', '7', 'утра']
    assert r[0].tags == ['O', 'O', 'B-when', 'I-when']
    assert r[1].tokens == ['будильник', 'на', '7', 'утра']
    assert r[1].tags == ['O', 'O', 'O', 'O']
    assert isinstance(r[2], Sample) and not r[2]


@pytest.mark.parametrize("processors, output, error", [
    ({}, ['need', 'an', 'inexpensive', 'flight,', 'from', 'baltimore', 'to', 'san', 'francisco!'], None),
    ({'normalizer'}, ['need', 'an', 'inexpensive', 'flight', 'from', 'baltimore', 'to', 'san', 'francisco'], None),
    (None, [], TestSampleProcessorError),
    ({'failing'}, [], TestSampleProcessorError),
    ({'unknown'}, [], SamplesExtractorError),
])
def test_samples_extractor_partial_appliance(normalize_sample_processor, processors, output, error):
    samples_extractor = SamplesExtractor([
        normalize_sample_processor,
        TestFailingSampleProcessor()
    ])
    item = NluSourceItem('need an inexpensive flight, from baltimore to san francisco!')
    if error is None:
        assert samples_extractor([item], sample_processor_names=processors)[0].tokens == output
    else:
        with pytest.raises(error):
            samples_extractor([item], sample_processor_names=processors)


def test_no_profanity(samples_extractor):
    r = samples_extractor(['нельзя просто так взять и заменить хуй на <censored>!'])
    assert 'хуй' in r[0].tokens


def test_no_url_replacement(samples_extractor):
    r = samples_extractor(
        FuzzyNLUFormat.parse_iter(["открой 'ютуб точка ком'(query)'"]).items
    )[0]
    assert r.tokens == ['открой', 'ютуб', 'точка', 'ком']
    assert r.tags == ['O', 'B-query', 'I-query', 'I-query']


@pytest.mark.parametrize("input, output", [
    ('%долой ;лишние$ символы!', 'долой лишние $ символы'),
    ('долой(*& @)(#лишние(%*)$ ))!()(символы$&*(!#', 'долой лишние $ символы $'),
    ('-долой- +лишние+ +-символы+- +1', 'долой лишние символы + 1'),
    ('Кто придумал c++', 'кто придумал c++'),
    ('Кто придумал с++', 'кто придумал с++'),
])
def test_strip_special(input, output, samples_extractor):
    r = samples_extractor([
        input
    ])[0]
    assert ' '.join(r.tokens) == output


@pytest.mark.parametrize("input, output", [
    ("2 + 2", "2 + 2"),
    ("2 * 2", "2 * 2"),
    ("2 - 2", "2 - 2"),
    ("2 / 2", "2 / 2"),
    ("2^2", "2 ^ 2"),
    ("2 ^ 2", "2 ^ 2"),
    ("два плюс два", "2 плюс 2"),
    ("2+2", "2 + 2"),
    ("-2+2= 0", "-2 + 2 = 0"),
    ("-2+2=0", "-2 + 2 = 0"),
    ("+2=2", "+ 2 = 2"),
    ("2-2", "2 - 2"),
    ("2*2", "2 * 2"),
    ("2/2", "2 / 2"),
    ("1 целая - 2 5", "1 целая - 2 5"),
    ("проверить номер 8(952)518-87-88", "проверить номер 8 952 518 - 87 - 88"),
    ("большой родничок у ребенка 4-4,5 см", "большой родничок у ребенка 4 - 4,5 см"),
    ("спорт-экспресс свежий номер-экспресс-экспресс", "спорт-экспресс свежий номер-экспресс-экспресс"),
    ("2-2*2*2-2_2", "2 - 2 * 2 * 2 - 2 2"),
])
def test_arithmetic(input, output, samples_extractor):
    r = samples_extractor([input])[0]
    assert ' '.join(r.tokens) == output


@pytest.mark.parametrize("input, output", [
    ("21/12/2012", "21/12/2012"),
    ("21-12-2012", "21-12-2012"),
    ("21 / 12 / 2012", "21 / 12 / 2012"),
    ("21 - 12 - 2012", "21 - 12 - 2012"),
])
def test_date(input, output, samples_extractor):
    r = samples_extractor([input])[0]
    assert ' '.join(r.tokens) == output


@pytest.mark.parametrize("input, output", [
    ("мясо-молочная", "мясо-молочная"),
    ("и/или", "и/или"),
    ("дом/9", "дом 9"),
    ("5-ого", "5 ого"),
    ("арзамас-16", "арзамас 16"),
    ("арзамас-16.5", "арзамас 16.5"),
    ("вова+1", "вова 1")
])
def test_composed_word(input, output, samples_extractor):
    r = samples_extractor([input])[0]
    assert ' '.join(r.tokens) == output


@pytest.mark.parametrize("input, output", [
    ("дай 100$", "дай 100 $"),
    ("дай 100 $", "дай 100 $"),
    ("дай 100€", "дай 100 €"),
    ("дай 100 €", "дай 100 €"),
    ("дай 100 €", "дай 100 €"),
    ("200$-200€", "200 $ 200 €"),
])
def test_currency(input, output, samples_extractor):
    r = samples_extractor([input])[0]
    assert ' '.join(r.tokens) == output


@pytest.mark.parametrize("input, tokens, tags", zip(
    FuzzyNLUFormat.parse_iter([
        'курс "100"(amount) "$"(currency)"'
    ]).items, [
        'курс 100 $'
    ], [
        'O B-amount B-currency'
    ]
))
def test_fuzzy_nlu_format_normalization(input, tokens, tags, samples_extractor):
    r = samples_extractor([input])[0]
    assert ' '.join(r.tokens) == tokens
    assert ' '.join(r.tags) == tags


def _get_multitype_items(prefix):
    return [
        FuzzyNLUFormat.parse_iter([prefix + ' "fuzzynluformat"(type)']).items[0],
        prefix + ' string',
        Utterance(prefix + ' utterance'),
        None
    ]


@pytest.fixture(scope='module')
def sample_cache(tmpdir_factory, samples_extractor):
    tempdir = tmpdir_factory.mktemp('sample_cache')
    sample_cache_file = str(tempdir.join('sample_cache.pkl'))
    samples_extractor(_get_multitype_items('кеш'), sample_cache=sample_cache_file)
    yield sample_cache_file
    tempdir.remove()


@pytest.mark.parametrize("num_procs", (1, 2, 10))
@pytest.mark.skip(reason="Hangs for an unknown reason")
def test_with_sample_cache(mocker, sample_cache, samples_extractor, num_procs):
    dummy_non_cached = 'dummy non cached'
    process_sample_mock = mocker.patch.object(
        vins_core.nlu.samples_extractor.SamplesExtractor, '_process_sample',
        return_value=Sample.from_string(dummy_non_cached)
    )
    samples = samples_extractor(
        items=_get_multitype_items('кеш') + _get_multitype_items('новый %d' % num_procs),
        sample_cache=sample_cache,
        num_procs=num_procs
    )
    if num_procs == 1:
        assert process_sample_mock.call_count == 3
    assert Counter(sample.default.text for sample in samples)[dummy_non_cached] == 3
