# coding: utf-8
from __future__ import unicode_literals

import pytest
import json

from vins_core.common.annotations import WizardAnnotation
from vins_core.common.entity import Entity
from vins_core.common.sample import Sample
from vins_core.dm.formats import NluSourceItem
from vins_core.ner.fst_normalizer import NluFstNormalizer
from vins_core.utils.lemmer import Inflector, GRAM_CASE, GRAM_GENDER
from vins_core.nlu.samples_extractor import SamplesExtractor


@pytest.fixture(scope='module')
def samples_extractor(normalizing_samples_extractor):
    return {
        'ru_nonorm': SamplesExtractor(),
        'ru': normalizing_samples_extractor
    }


def test_fst_normalizer_unknown_symbols():
    known = (r'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ' +
             r'абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ' +
             r'äáàâãāçëéèêẽēğıïíìîĩīöóòôõōşüúùûũūßœæÿñ' +
             r'ÄÁÀÂÃĀÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪŒÆŸÑ' +
             r'ґієїҐІЄЇ¿¡' +
             r' \t\r\n\u00a0' +
             r'!"#%&\'()*+,-./0123456789:;<=>?@\[\]\\\^_`{|}~’‘´ ́ ̀' +
             r'±×€™§©®$‰№«»•’“”…₽£¢₴₺£₤¥‐‑‒–—―' +
             r'\u0302\u0306\u0308\u0327')
    some_unknown = 'βασιλείαῥωμαίων'
    assert not any(map(NluFstNormalizer.has_unknown_symbol, known))
    assert all(map(NluFstNormalizer.has_unknown_symbol, some_unknown))


class TestNluFst(object):
    NAME = ''
    LANG = 'ru'

    @classmethod
    def __call__(cls, parser, samples_extractor, utterance, samples_extractor_key=None, annotations=None):
        if not samples_extractor_key:
            samples_extractor_key = cls.LANG
        utt = ' '.join(
            samples_extractor[samples_extractor_key](
                [NluSourceItem(utterance)], filter_errors=True)[0].tokens
        )

        # Old Sample implementation added boseos tags to .tokens and .tags.
        # So tests were written with answers shifted by +1.
        # %TODO: Without boseos tags TestFio::test_surname_initials causes. Segmentation fault =).
        # %TODO: Something wrong with parsers.
        utt = '<s> ' + utt + ' </s>'
        sample = Sample.from_string(utt, annotations=annotations)
        parser_output = parser(sample)
        # try json serializer
        assert json.dumps([e.to_dict() for e in parser_output])
        if cls.NAME:
            return [item for item in parser_output if item.type == cls.NAME]
        else:
            return parser_output


class TestNluFstDatetimeRu(TestNluFst):
    NAME = 'DATETIME'

    def test_time_half_past_four(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'половина пятого вечера')
        assert r == [Entity(
            start=1,
            end=4,
            type='DATETIME',
            value={'hours': 16, 'minutes': 30}
        )]

    @pytest.mark.parametrize("input, output", [
        (
            'через год и три с половиной месяца',
            Entity(
                start=1,
                end=6,
                type='DATETIME',
                value={
                    'years': 1,
                    'years_relative': True,
                    'months': 3,
                    'months_relative': True,
                    'weeks': 2,
                    'weeks_relative': True
                }
            ),
        ),
        (
            'через два года',
            Entity(
                start=1,
                end=4,
                type='DATETIME',
                value={'years': 2, 'years_relative': True}
            )
        )
    ])
    def test_relative_date(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input)[0] == output

    def test_relative_time(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'через 3 часа, 9 минут, 15 секунд приедет поезд')[0]
        assert r == Entity(
            start=1,
            end=8,
            type='DATETIME',
            value={
                'time_relative': True,
                'hours': 3,
                'minutes': 9,
                'seconds': 15
            }
        )

    def test_time_relative_day(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на без 15 6 завтра')
        assert r == [Entity(
            start=3,
            end=7,
            type='DATETIME',
            value={
                'hours': 5,
                'days_relative': True,
                'minutes': 45,
                'days': 1
            }
        )]

    def test_time_with_left_right_context(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'встретимся послезавтра в без 15 6 вечера')
        assert r == [Entity(
            start=2,
            end=8,
            type='DATETIME',
            value={
                'days': 2,
                'days_relative': True,
                'hours': 17,
                'minutes': 45
            }
        )]

    def test_time_with_left_right_context_unnorm(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'встретимся послезавтра, в без пятнадцати шесть вечера')
        assert r == [Entity(
            start=2,
            end=8,
            type='DATETIME',
            value={
                'days': 2,
                'days_relative': True,
                'hours': 17,
                'minutes': 45
            }
        )]

    def test_time_context_with_prep(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'встретимся послезавтра приблизительно в 17 10')
        assert r == [Entity(
            start=2,
            end=7,
            type='DATETIME',
            value={
                'days': 2,
                'days_relative': True,
                'hours': 17,
                'minutes': 10
            }
        )]

    @pytest.mark.parametrize("input, output", [
        ('встретимся послезавтра, около десяти минут шестого вечера!', [Entity(
            start=2,
            end=8,
            type='DATETIME',
            value={
                'days': 2,
                'days_relative': True,
                'hours': 17,
                'minutes': 10
            }
        )]),
        ('ноль минут ноль секунд', [])
    ])
    def test_time_context_with_prep_unnorm(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input) == output

    @pytest.mark.parametrize("input, output", [
        ('праздник победы 9 мая 1945 го года был 2 недели назад', [Entity(
            start=3,
            end=8,
            type='DATETIME',
            value={
                'months': 5,
                'days': 9,
                'years': 1945
            }
        ), Entity(
            start=9,
            end=12,
            type='DATETIME',
            value={
                'weeks': -2,
                'weeks_relative': True
            }
        )]),
        ('пару лет назад', [Entity(start=1, end=4, type='DATETIME', value={
            'years': -2, 'years_relative': True
        })]),
        ('через пару лет', [Entity(start=1, end=4, type='DATETIME', value={
            'years': 2, 'years_relative': True
        })])
    ])
    def test_date_and_relative_date(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input) == output

    @pytest.mark.parametrize("input, output", [
        ('погода в следующее воскресенье', {'weeks': 1, 'weeks_relative': True, 'weekday': 7}),
        ('дата прошлого четверга', {'weeks': -1, 'weeks_relative': True, 'weekday': 4}),
        ('к этой среде', {'weekday': 3}),
    ])
    def test_weekday(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input)[0].value == output

    def test_date_and_time_formatted_unnorm(self, parser, samples_extractor):
        r = self(parser, samples_extractor, '08.03.2016, в 10:30 будет праздник!!')
        assert r == [Entity(
            start=1,
            end=4,
            type='DATETIME',
            value={'hours': 10, 'months': 3, 'minutes': 30, 'days': 8, 'years': 2016}
        )]

    @pytest.mark.parametrize("phrase, values", [
        ("на 5 декабря 2016", [{'months': 12, 'days': 5, 'years': 2016}]),
        ("в ноябре 2014 года", [{'months': 11, 'years': 2014}]),
        ('концерт субботний вечер 23 сентября 17 года', [{'days': 23, 'months': 9, 'years': 2017}]),
        ('программа передач на вечер 1 октября 1 канала', [{'months': 10, 'days': 1}]),
        ('восемьсот пятьдесят третий год москва что за день', [{'years': 853}]),
        # DIALOG-912
        ('обзор игрового дня 14 10 17 год', [{'years': 2017}]),
        ('при какой день 5 6 301 года месяц октябрь ноябрь февраль март апрель июль август сентябрь', [
            {'years': 301}, {'months': 10}, {'months': 11}, {'months': 2}, {'months': 3}, {'months': 4},
            {'months': 7}, {'months': 8}, {'months': 9}
        ]),
        ('надо амине амине идет в школу 8 год 8 5 день 21 года', [
            {'years': 2008}, {'years': 2021}
        ]),
        ('внести программу в виде цифры хотя бы что там должно быть на другой барахты его 96 год в 1 3 дня и 91 4', [
            {'years': 1996}
        ]),
        ('канал россия 1 сегодня 15 октября 17 года программа канал россия 1', [
            {'hours': 1, 'months': 10, 'minutes': 0, 'days': 15, 'years': 2017}
        ]),
        ('какой день недели 1 января следующего года', [
            {'days': 1, 'months': 1, 'years': 1, 'years_relative': True}
        ]),
        ('в следующем году', [{'years': 1, 'years_relative': True}]),
        ('будильник на 11:30 10 марта 2018 года',
         [{'hours': 11, 'minutes': 30, 'days': 10, 'months': 3, 'years': 2018}]),
        ('8 утра 24 октября',
         [{'hours': 8, 'minutes': 0, 'days': 24, 'months': 10}]),
        ('на 29 декабря в 7 вечера', [
            {'months': 12, 'days': 29, 'hours': 19, 'minutes': 0}
        ]),
        ('в 7 вечера на 29 декабря', [
            {'months': 12, 'days': 29, 'hours': 19, 'minutes': 0}
        ]),
        # json parser
        ('вспомни седьмой год', [{'years': 2007}]),
        ('сороковой год', [{'years': 2040}]),
        ('сорок первый год', [{'years': 1941}]),
        ('восьмидесятый год', [{'years': 1980}])
    ])
    def test_date(self, parser, samples_extractor, phrase, values):
        results = self(parser, samples_extractor, phrase)
        for i, result in enumerate(results):
            assert result.value == values[i]

    def test_two_objects(self, parser, samples_extractor):
        r = self(parser, samples_extractor, '1 мая - день труда, в 3 после полудня на картошку .')
        assert r == [Entity(start=1, end=3, type='DATETIME', value={'months': 5, 'days': 1}),
                     Entity(start=7, end=10, type='DATETIME', value={'hours': 15, 'minutes': 0})]

    def test_halfpast_evening(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на полшестого вечера')
        assert r == [Entity(
            start=3,
            end=5,
            type='DATETIME',
            value={
                'hours': 17,
                'minutes': 30
            }
        )]

    def test_halfpast_morning(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на полшестого утра')
        assert r == [Entity(
            start=3,
            end=5,
            type='DATETIME',
            value={
                'hours': 5,
                'minutes': 30
            }
        )]

    def test_time_morning(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на 6 30 утра')
        assert r == [Entity(
            start=3,
            end=6,
            type='DATETIME',
            value={
                'hours': 6,
                'minutes': 30
            }
        )]

    def test_time_formatted_morning(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на 6:30 утра')
        assert r == [Entity(
            start=3,
            end=5,
            type='DATETIME',
            value={
                'hours': 6,
                'minutes': 30
            }
        )]

    def test_time_evening_context(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину к вечеру на шесть сорок пять ')
        assert r == [Entity(start=3, end=7, type='DATETIME', value={'hours': 18, 'minutes': 45})]

    def test_time_with_words(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину на 7 часов 30 минут')
        assert r == [
            Entity(
                start=3,
                end=7,
                type='DATETIME',
                value={
                    'hours': 7,
                    'minutes': 30
                }
            )
        ]

    def test_time_morning_relative_day(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'будильник на 6 утра завтра')
        assert r == [
            Entity(start=3, end=6, type='DATETIME',
                   value={'hours': 6, 'minutes': 0, 'days_relative': True, 'days': 1})]

    def test_time_evening_relative_day_inserted_prep(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'были там позавчера вечером примерно около девяти тридцати')
        assert r == [Entity(start=3, end=9, type='DATETIME',
                            value={'hours': 21, 'days_relative': True, 'minutes': 30, 'days': -2})]

    def test_noon(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'машину в полдень')
        assert r == [Entity(
            start=3,
            end=4,
            type='DATETIME',
            value={
                'hours': 12,
                'minutes': 0
            }
        )]

    def test_midnight(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'мне нужно такси в полночь')
        assert r == [Entity(
            start=5,
            end=6,
            type='DATETIME',
            value={
                'hours': 0,
                'minutes': 0
            }
        )]

    def test_yesterday_time(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'вчера в 10 минут первого утра')
        assert r == [Entity(
            start=1,
            end=7,
            type='DATETIME',
            value={
                'hours': 0,
                'minutes': 10,
                'days': -1,
                'days_relative': True,
            }
        )]

    def test_weekday_time(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "в следующую субботу в без пятнадцати шесть вечера")
        assert r == [Entity(
            start=2,
            end=9,
            type='DATETIME',
            value={'hours': 17, 'minutes': 45, 'weekday': 6, 'weeks': 1, 'weeks_relative': True}
        )]

    def test_weekday_time_past(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "четверть восьмого утра в прошлую среду")
        assert r == [Entity(
            start=1,
            end=7,
            type='DATETIME',
            value={'hours': 7, 'weeks': -1, 'minutes': 15, 'weekday': 3, 'weeks_relative': True}
        )]

    def test_weekday_time_present(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "четверть восьмого утра в среду")
        assert r == [Entity(
            start=1,
            end=6,
            type='DATETIME',
            value={'hours': 7, 'minutes': 15, 'weekday': 3}
        )]

    def test_weekday_time_present_ctx(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "четверть восьмого утра в эту среду")
        assert r == [Entity(
            start=1,
            end=4,
            type='DATETIME',
            value={'hours': 7, 'minutes': 15}
        ), Entity(
            start=6,
            end=7,
            type='DATETIME',
            value={'weekday': 3}
        )]

    def test_weekday_time_present_ctx_2(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "в ближайшую среду на четверть восьмого утра")
        assert r == [Entity(
            start=2,
            end=8,
            type='DATETIME',
            value={'hours': 7, 'minutes': 15, 'weekday': 3}
        )]

    def test_hours_formatted_no_mins(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'давай лучше в 20:00')
        assert r == [Entity(
            start=4,
            end=5,
            type='DATETIME',
            value={u'hours': 20, u'minutes': 0}
        )]

    @pytest.mark.xfail(reason='HH MM format only supported within time context')
    def test_twenty_hours_by_words(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'в двадцать ноль ноль')
        assert r == [Entity(start=2, end=4, type='DATETIME', value={'hours': 20, 'minutes': 0})]

    @pytest.mark.xfail(reason='HH MM format only supported within time context')
    def test_hours_mins_utt(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'в двадцать один тридцать два')
        assert r == [Entity(start=2, end=4, type='DATETIME', value={'hours': 21, 'minutes': 32})]

    @pytest.mark.parametrize("phrase,value", [
        ("через полчаса", {'minutes': 30, 'minutes_relative': True}),
        ("через полтора часа", {'hours': 1, 'hours_relative': True, 'minutes': 30, 'minutes_relative': True}),
        ("через полминуты", {'seconds': 30, 'seconds_relative': True}),
        ("через полторы минуты", {'minutes': 1, 'minutes_relative': True, 'seconds': 30, 'seconds_relative': True}),
        ("полчаса назад", {'minutes': -30, 'minutes_relative': True}),
        ("полтора часа назад", {'hours': -1, 'hours_relative': True, 'minutes': -30, 'minutes_relative': True}),
        ("полминуты назад", {'seconds': -30, 'seconds_relative': True}),
        ("полторы минуты назад", {'minutes': -1, 'minutes_relative': True, 'seconds': -30, 'seconds_relative': True}),
        ('через пару часов', {'hours': 2, 'hours_relative': True}),
        ('пару минут назад', {'minutes': -2, 'minutes_relative': True})
    ])
    def test_special_hours_mins(self, parser, samples_extractor, phrase, value):
        assert self(parser, samples_extractor, phrase)[0].value == value

    @pytest.mark.parametrize("phrase, value", [
        ('яндекс сейчас', {'seconds': 0, 'seconds_relative': True}),
        ('погода в настоящее время', {'seconds': 0, 'seconds_relative': True})
    ])
    def test_now(self, parser, samples_extractor, phrase, value):
        assert self(parser, samples_extractor, phrase)[0].value == value

    @pytest.mark.parametrize("phrase, value", [
        ('завтра на 8 утра', {"days": 1, "days_relative": True, 'hours': 8, 'minutes': 0}),
        ('завтра на 8', {"days": 1, "days_relative": True, 'hours': 8, 'minutes': 0}),
        ('на 8', []),
        ('на 8 с половиной утра', {'hours': 8, 'minutes': 30}),
        ('на час дня', {'hours': 13, 'minutes': 0}),
        ('на два дня', []),
        ('на два часа дня', {'hours': 14, 'minutes': 0}),
        ('суббота 5 число или нет', {'weekday': 6, 'days': 5})
    ])
    def test_time_mandatory_context(self, parser, samples_extractor, phrase, value):
        r = self(parser, samples_extractor, phrase)
        if len(r) > 0:
            assert r[0].value == value
        else:
            assert r == value

    @pytest.mark.parametrize("phrase, value", [
        ("8 с половиной вечера", {'hours': 20, 'minutes': 30}),
        ("завтра в 8 с половиной вечера", {'hours': 20, 'minutes': 30, 'days': 1, 'days_relative': True}),
        ("8 с половиной часов вечера", {'hours': 20, 'minutes': 30}),
        ("завтра в 8 с половиной часов вечера", {'hours': 20, 'minutes': 30, 'days': 1, 'days_relative': True}),
    ])
    def test_time_hours_and_half(self, parser, samples_extractor, phrase, value):
        assert self(parser, samples_extractor, phrase)[0].value == value

    @pytest.mark.parametrize("phrase, value", [
        ('на завтра', {'days': 1, 'days_relative': True}),
        ('погода на завтрашний день', {'days': 1, 'days_relative': True}),
        ('послезавтрашняя погода', {'days': 2, 'days_relative': True}),
        ('завтрашняя погода', {'days': 1, 'days_relative': True}),
        ('вчерашняя погода', {'days': -1, 'days_relative': True}),
        ('позавчерашняя погода', {'days': -2, 'days_relative': True}),
        ('погода к завтрашнему дню', {'days': 1, 'days_relative': True}),
        ('что вчера было', {'days': -1, 'days_relative': True}),
        ('курс за вчерашний день', {'days': -1, 'days_relative': True}),
        ('вчера вечером в пять', {'days': -1, 'days_relative': True, 'hours': 17, 'minutes': 0}),
        ('утро вчерашнего дня', {'days': -1, 'days_relative': True}),
    ])
    def test_time_spec_day(self, parser, samples_extractor, phrase, value):
        assert self(parser, samples_extractor, phrase)[0].value == value


class TestNluFstDate(TestNluFst):
    NAME = 'DATE'

    @pytest.mark.parametrize("input, output", [
        ('через год и три с половиной месяца',
         [Entity(start=1, end=6, type='DATE', value={
             'years': 1, 'years_relative': True, 'months': 3,
             'months_relative': True, 'weeks': 2, 'weeks_relative': True})]),
        ('через два года',
         [Entity(start=1, end=4, type='DATE', value={'years': 2, 'years_relative': True})]),
        ('встретимся послезавтра, около десяти минут шестого вечера!',
         [Entity(start=2, end=3, type='DATE', value={'days': 2, 'days_relative': True})]),
        ('ноль минут ноль секунд', []),
        ('машину на без 15 6 завтра',
         [Entity(start=6, end=7, type='DATE', value={'days_relative': True, 'days': 1})]),
        ('встретимся послезавтра в без 15 6 вечера',
         [Entity(start=2, end=3, type='DATE', value={'days': 2, 'days_relative': True})]),
        ('встретимся послезавтра, в без пятнадцати шесть вечера',
         [Entity(start=2, end=3, type='DATE', value={'days': 2, 'days_relative': True})]),
        ('встретимся послезавтра приблизительно в 17 10',
         [Entity(start=2, end=3, type='DATE', value={'days': 2, 'days_relative': True})]),
        ('праздник победы 9 мая 1945 го года был 2 недели назад',
         [Entity(start=3, end=8, type='DATE', value={'months': 5, 'days': 9, 'years': 1945}),
          Entity(start=9, end=12, type='DATE', value={'weeks': -2, 'weeks_relative': True})]),
        ('пару лет назад',
         [Entity(start=1, end=4, type='DATE', value={'years': -2, 'years_relative': True})]),
        ('через пару лет',
         [Entity(start=1, end=4, type='DATE', value={'years': 2, 'years_relative': True})]),
        ('08.03.2016, в 10:30 будет праздник!!',
         [Entity(start=1, end=2, type='DATE', value={'months': 3, 'days': 8, 'years': 2016})]),
        ('1 мая - день труда, в 3 после полудня на картошку .',
         [Entity(start=1, end=3, type='DATE', value={'months': 5, 'days': 1})]),
        ('будильник на 6 утра завтра',
         [Entity(start=5, end=6, type='DATE', value={'days_relative': True, 'days': 1})]),
        ('были там позавчера вечером примерно около девяти тридцати',
         [Entity(start=3, end=4, type='DATE', value={'days_relative': True, 'days': -2})]),
        ('вчера в 10 минут первого утра',
         [Entity(start=1, end=2, type='DATE', value={'days': -1, 'days_relative': True})]),
        ('в следующую субботу в без пятнадцати шесть вечера',
         [Entity(start=2, end=4, type='DATE', value={'weekday': 6, 'weeks': 1, 'weeks_relative': True})]),
        ('четверть восьмого утра в прошлую среду',
         [Entity(start=5, end=7, type='DATE', value={'weeks': -1, 'weekday': 3, 'weeks_relative': True})]),
        ('четверть восьмого утра в среду',
         [Entity(start=5, end=6, type='DATE', value={'weekday': 3})]),
        ('четверть восьмого утра в эту среду',
         [Entity(start=6, end=7, type='DATE', value={'weekday': 3})]),
        ('в ближайшую среду на четверть восьмого утра',
         [Entity(start=2, end=4, type='DATE', value={'weekday': 3})]),
        ('Какой день недели 1 января следующего года', [Entity(
            start=4,
            end=8,
            type='DATE',
            value={'days': 1, 'months': 1, 'years': 1, 'years_relative': True}
        )]),
        ('покажи лучшую комедию 2014', [Entity(start=4, end=5, type='DATE', value={'years': 2014})]),
        ('год спустя', []),
        ('5 лет спустя', []),
        ('5 лет назад', [Entity(start=1, end=4, type='DATE', value={'years': -5, 'years_relative': True})])
    ])
    def test_date_full_markup(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input) == output

    @pytest.mark.parametrize("phrase, values", [
        ("на 5 декабря 2016", [{'months': 12, 'days': 5, 'years': 2016}]),
        ("в ноябре 2014 года", [{'months': 11, 'years': 2014}]),
        ('концерт субботний вечер 23 сентября 17 года', [{'days': 23, 'months': 9, 'years': 2017}]),
        ('программа передач на вечер 1 октября 1 канала', [{'months': 10, 'days': 1}]),
        ('восемьсот пятьдесят третий год москва что за день', [{'years': 853}]),
        # DIALOG-912
        ('обзор игрового дня 14 10 17 год', [{'years': 2017}]),
        ('при какой день 5 6 301 года месяц октябрь ноябрь февраль март апрель июль август сентябрь', [
            {'years': 301}, {'months': 10}, {'months': 11}, {'months': 2}, {'months': 3}, {'months': 4},
            {'months': 7}, {'months': 8}, {'months': 9}
        ]),
        ('надо амине амине идет в школу 8 год 8 5 день 21 года', [
            {'years': 2008}, {'years': 2021}
        ]),
        ('внести программу в виде цифры хотя бы что там должно быть на другой барахты его 96 год в 1 3 дня и 91 4', [
            {'years': 1996}
        ]),
        ('канал россия 1 сегодня 15 октября 17 года программа канал россия 1', [
            {'months': 10, 'days_relative': True, 'days': 15, 'years': 2017}
        ]),
        # weekday
        ('погода в следующее воскресенье', [{'weeks': 1, 'weeks_relative': True, 'weekday': 7}]),
        ('дата прошлого четверга', [{'weeks': -1, 'weeks_relative': True, 'weekday': 4}]),
        ('к этой среде', [{'weekday': 3}]),
        # context
        ('завтра на 8 утра', [{"days": 1, "days_relative": True}]),
        ('завтра на 8', [{"days": 1, "days_relative": True}]),
        ('на 8', []),
        ('на 8 с половиной утра', []),
        ('на час дня', []),
        ('на два дня', []),
        ('на два часа дня', []),
        ('суббота 5 число или нет', [{'weekday': 6, 'days': 5}]),
        # specday
        ('на завтра', [{'days': 1, 'days_relative': True}]),
        ('погода на завтрашний день', [{'days': 1, 'days_relative': True}]),
        ('погода к завтрашнему дню', [{'days': 1, 'days_relative': True}]),
        ('что вчера было', [{'days': -1, 'days_relative': True}]),
        ('курс за вчерашний день', [{'days': -1, 'days_relative': True}]),
        ('вчера вечером в пять', [{'days': -1, 'days_relative': True}]),
        ('утро вчерашнего дня', [{'days': -1, 'days_relative': True}]),
        ('позавчерашная погода', [{'days': -2, 'days_relative': True}]),
        ('сегодняшние новости', [{'days': 0, 'days_relative': True}]),
        ('послезавтрашняя погода', [{'days': 2, 'days_relative': True}]),
        # json parser
        ('вспомни седьмой год', [{'years': 2007}]),
        ('сороковой год', [{'years': 2040}]),
        ('сорок первый год', [{'years': 1941}]),
        ('восьмидесятый год', [{'years': 1980}])
    ])
    def test_date(self, parser, samples_extractor, phrase, values):
        results = self(parser, samples_extractor, phrase)
        for i, result in enumerate(results):
            assert result.value == values[i]


class TestNluFstTime(TestNluFst):
    NAME = 'TIME'

    @pytest.mark.parametrize("input, output", [
        ('встретимся послезавтра, около десяти минут шестого вечера!',
         [Entity(start=4, end=8, type='TIME', value={'hours': 5, 'minutes': 10, 'period': 'pm'})]),
        ('ноль минут ноль секунд', [Entity(start=1, end=5, type='TIME', value={'seconds': 0, 'minutes': 0})]),
        ('половина пятого вечера',
         [Entity(start=1, end=4, type='TIME', value={'hours': 4, 'minutes': 30, 'period': 'pm'})]),
        ('встретимся послезавтра приблизительно в 17 10',
         [Entity(start=5, end=7, type='TIME', value={'hours': 17, 'minutes': 10})]),
        ('встретимся послезавтра, в без пятнадцати шесть вечера',
         [Entity(start=4, end=8, type='TIME', value={'hours': 5, 'minutes': 45, 'period': 'pm'})]),
        ('встретимся послезавтра в без 15 6 вечера',
         [Entity(start=4, end=8, type='TIME', value={'hours': 5, 'minutes': 45, 'period': 'pm'})]),
        ('машину на без 15 6 завтра',
         [Entity(start=3, end=6, type='TIME', value={'hours': 5, 'minutes': 45})]),
        ('через 3 часа, 9 минут, 15 секунд приедет поезд',
         [Entity(
             start=1,
             end=8,
             type='TIME',
             value={'time_relative': True, 'hours': 3, 'minutes': 9, 'seconds': 15}
         )]),
        ('08.03.2016, в 10:30 будет праздник!!',
         [Entity(start=3, end=4, type='TIME', value={'hours': 10, 'minutes': 30})]),
        pytest.param(
            'первого мая - день труда, в 3 после полудня на картошку .',
            [Entity(start=7, end=10, type='TIME', value={'hours': 3, 'period': 'pm'})],
            marks=pytest.mark.xfail(reason='could be fixed after DIALOG-1324')),
        ('машину на полшестого вечера',
         [Entity(start=3, end=5, type='TIME', value={'hours': 5, 'minutes': 30, 'period': 'pm'})]),
        ('машину на полшестого утра',
         [Entity(start=3, end=5, type='TIME', value={'hours': 5, 'minutes': 30, 'period': 'am'})]),
        ('машину на 6 30 утра',
         [Entity(start=3, end=6, type='TIME', value={'hours': 6, 'minutes': 30, 'period': 'am'})]),
        ('машину на 6:30 утра',
         [Entity(start=3, end=5, type='TIME', value={'hours': 6, 'minutes': 30, 'period': 'am'})]),
        ('машину к вечеру на шесть сорок пять ',
         [Entity(start=3, end=7, type='TIME', value={'hours': 6, 'minutes': 45, 'period': 'pm'})]),
        ('машину на 7 часов 30 минут',
         [Entity(start=3, end=7, type='TIME', value={'hours': 7, 'minutes': 30})]),
        ('будильник на 6 утра завтра',
         [Entity(start=3, end=5, type='TIME', value={'hours': 6, 'period': 'am'})]),
        ('были там позавчера вечером примерно около девяти тридцати',
         [Entity(start=4, end=9, type='TIME', value={'hours': 9, 'minutes': 30, 'period': 'pm'})]),
        ('машину в полдень',
         [Entity(start=3, end=4, type='TIME', value={'hours': 12, 'minutes': 0, 'period': 'pm'})]),
        ('мне нужно такси в полночь',
         [Entity(start=5, end=6, type='TIME', value={'hours': 12, 'minutes': 0, 'period': 'am'})]),
        ('вчера в 10 минут первого утра',
         [Entity(start=3, end=7, type='TIME', value={'hours': 12, 'minutes': 10, 'period': 'pm'})]),
        ('в следующую субботу в без пятнадцати шесть вечера',
         [Entity(start=5, end=9, type='TIME', value={'hours': 5, 'minutes': 45, 'period': 'pm'})]),
        ('четверть восьмого утра в прошлую среду',
         [Entity(start=1, end=4, type='TIME', value={'hours': 7, 'minutes': 15, 'period': 'am'})]),
        ('четверть восьмого утра в среду',
         [Entity(start=1, end=4, type='TIME', value={'hours': 7, 'minutes': 15, 'period': 'am'})]),
        ('четверть восьмого утра в эту среду',
         [Entity(start=1, end=4, type='TIME', value={'hours': 7, 'minutes': 15, 'period': 'am'})]),
        ('в ближайшую среду на четверть восьмого утра',
         [Entity(start=5, end=8, type='TIME', value={'hours': 7, 'minutes': 15, 'period': 'am'})]),
        ('давай лучше в 20:00',
         [Entity(start=4, end=5, type='TIME', value={'hours': 20, 'minutes': 0})]),
        ('в двадцать ноль ноль',
         [Entity(start=2, end=4, type='TIME', value={'hours': 20, 'minutes': 0})]),
        ('в двадцать один тридцать два',
         [Entity(start=2, end=4, type='TIME', value={'hours': 21, 'minutes': 32})]),
        ('в первом часу дня',
         [Entity(start=2, end=5, type='TIME', value={'hours': 1, 'period': 'pm'})]),
        ('во втором часу дня',
         [Entity(start=2, end=5, type='TIME', value={'hours': 2, 'period': 'pm'})]),
        ('Поставь таймер на 5 минут 30 секунд',
         [Entity(start=4, end=8, type='TIME', value={'minutes': 5, 'seconds': 30})]),
        ('Напомни через 2 с половиной минуты',
         [Entity(start=2, end=5, type='TIME',
                 value={'minutes': 2, 'minutes_relative': True, 'seconds': 30, 'seconds_relative': True})]),
        ('Установи время на 13 часов 20 минут 42 секунды',
         [Entity(start=4, end=10, type='TIME', value={'hours': 13, 'minutes': 20, 'seconds': 42})]),
        ('Через 2 минуты и 30 секунд напомни',
         [Entity(start=1, end=7, type='TIME',
                 value={'minutes': 2, 'minutes_relative': True, 'seconds': 30, 'seconds_relative': True})]),
        ('поставь будильник на половину восьмого',
         [Entity(start=4, end=6, type='TIME', value={'hours': 7, 'minutes': 30})]),
        ('поставь сегодня будильник в одиннадцать часов тридцать',
         [Entity(start=5, end=8, type='TIME', value={'hours': 11, 'minutes': 30})]),
        ('поставь напоминание через пол часа покушать еду',
         [Entity(start=3, end=6, type='TIME', value={'minutes': 30, 'minutes_relative': True})]),
        ('без пяти минут два',
         [Entity(start=1, end=5, type='TIME', value={'hours': 1, 'minutes': 55})]),
        ('без четверти час',
         [Entity(start=1, end=4, type='TIME', value={'hours': 0, 'minutes': 45})]),
        ('без двух минут час',
         [Entity(start=1, end=5, type='TIME', value={'hours': 0, 'minutes': 58})]),
        ('без половины час',
         [Entity(start=1, end=4, type='TIME', value={'hours': 0, 'minutes': 30})]),
        ('без двадцати 10',
         [Entity(start=1, end=4, type='TIME', value={'hours': 9, 'minutes': 40})]),
    ])
    def test_with_markup(self, parser, samples_extractor, input, output):
        assert self(parser, samples_extractor, input) == output

    @pytest.mark.parametrize("phrase, values", [
        # test alarm
        ('будильник на 6', [{'hours': 6}]),
        ('будильник на 6 45', [{'hours': 6, 'minutes': 45}]),
        ('будильник на 19 03', [{'hours': 19, 'minutes': 3}]),
        ('будильник на 19 0 3', [{'hours': 19, 'minutes': 3}]),
        ('будильник через 3 часа', [{'hours': 3, 'hours_relative': True}]),
        ('будильник на 15 минут', [{'minutes': 15}]),
        ('будильник на 3 часа 15 минут', [{'hours': 3, 'minutes': 15}]),
        ('будильник на 3 часа 05 минут', [{'hours': 3, 'minutes': 5}]),
        ('будильник на 3 часа 0 5 минут', [{'hours': 3, 'minutes': 5}]),
        ('будильник на 8 часов 15 минут вечера', [{'hours': 8, 'minutes': 15, 'period': 'pm'}]),
        ('будильник на час 30', [{'hours': 1, 'minutes': 30}]),
        ('будильник на час 30 утра', [{'hours': 1, 'minutes': 30, 'period': 'am'}]),
        ('будильник на час 0 2 утра', [{'hours': 1, 'minutes': 2, 'period': 'am'}]),
        # test timer
        ('таймер на 42 секунды', [{'seconds': 42}]),
        ('таймер на 3 минуты', [{'minutes': 3}]),
        ('таймер через 3 минуты', [{'minutes': 3, 'minutes_relative': True}]),
        ('таймер на 10 секунд', [{'seconds': 10}]),
        ('таймер через 10 секунд', [{'seconds': 10, 'seconds_relative': True}]),
        # special hours mins
        ("через полчаса", [{'minutes': 30, 'minutes_relative': True}]),
        ("через полтора часа", [{'hours': 1, 'hours_relative': True, 'minutes': 30, 'minutes_relative': True}]),
        ("через полминуты", [{'seconds': 30, 'seconds_relative': True}]),
        ("через полторы минуты", [{'minutes': 1, 'minutes_relative': True, 'seconds': 30, 'seconds_relative': True}]),
        ("полчаса назад", [{'minutes': -30, 'minutes_relative': True}]),
        ("полтора часа назад", [{'hours': -1, 'hours_relative': True, 'minutes': -30, 'minutes_relative': True}]),
        ("полминуты назад", [{'seconds': -30, 'seconds_relative': True}]),
        ("полторы минуты назад", [{'minutes': -1, 'minutes_relative': True, 'seconds': -30, 'seconds_relative': True}]),
        ('через пару часов', [{'hours': 2, 'hours_relative': True}]),
        ('пару минут назад', [{'minutes': -2, 'minutes_relative': True}]),
        # now
        ('яндекс сейчас', [{'seconds': 0, 'seconds_relative': True}]),
        ('погода в настоящее время', [{'seconds': 0, 'seconds_relative': True}]),
        # mandatory context
        ('завтра на 8 утра', [{'hours': 8, 'period': 'am'}]),
        ('завтра на 8', [{'hours': 8}]),
        ('на 8', [{'hours': 8}]),
        ('на 8 с половиной утра', [{'hours': 8, 'minutes': 30, 'period': 'am'}]),
        ('в 11 дня', [{'hours': 11, 'period': 'am'}]),
        ('в 12 дня', [{'hours': 12, 'period': 'pm'}]),
        ('на час дня', [{'hours': 1, 'period': 'pm'}]),
        ('на два дня', [{'hours': 2, 'period': 'pm'}]),
        ('на два часа дня', [{'hours': 2, 'period': 'pm'}]),
        ('8 с половиной вечера', [{'hours': 8, 'minutes': 30, 'period': 'pm'}]),
        ('завтра в 8 с половиной вечера', [{'hours': 8, 'minutes': 30, 'period': 'pm'}]),
        ('8 с половиной часов вечера', [{'hours': 8, 'minutes': 30, 'period': 'pm'}]),
        ('завтра в 8 с половиной часов вечера', [{'hours': 8, 'minutes': 30, 'period': 'pm'}]),
        ('вчера вечером в пять', [{'hours': 5, 'period': 'pm'}]),
        ('половина первого', [{'hours': 0, 'minutes': 30}]),
        ('половина первого ночи', [{'hours': 12, 'minutes': 30, 'period': 'am'}]),
        ('половина двенадцатого ночи', [{"hours": 11, "minutes": 30, "period": "pm"}]),
        ('четверть первого', [{'hours': 0, 'minutes': 15}]),
        ('четверть второго', [{'hours': 1, 'minutes': 15}]),
        ('первый час ночи', [{'hours': 1, 'period': 'am'}]),
        ('в первом часу дня', [{'hours': 1, 'period': 'pm'}]),
        ('во втором часу дня', [{'hours': 2, 'period': 'pm'}]),
        ('будильник на шесть pm', [{'hours': 6, 'period': 'pm'}]),
        ('программа ю беременна в 16 по телевизору 15 минут 0 8 показывают',
         [{'hours': 16}, {'minutes': 15, 'seconds': 8}]),
        ('4 минуты 0 4', [{'minutes': 4, 'seconds': 4}]),
        ('7 минут 0 5 минут', [{'minutes': 7, 'seconds': 0}, {'minutes': 5}]),
        ('оля пробежала дистанцию за 11 минут 0 5 секунд а таня опередила олю'
         ' на 10 секунд за какое время пробежала дистанцию таня реши задачу',
         [{'minutes': 11, 'seconds': 5}, {'seconds': 10}]),
        ('поставь будильник на половину восьмого', [{'hours': 7, 'minutes': 30}]),
        ('поставь будильник на полпервого', [{'hours': 0, 'minutes': 30}]),
        ('поставь будильник на полпервого утра', [{'hours': 12, 'minutes': 30, 'period': 'pm'}]),
        ('поставь будильник на полпервого дня', [{'hours': 12, 'minutes': 30, 'period': 'pm'}]),
        ('поставь будильник на полпервого ночи', [{'hours': 12, 'minutes': 30, 'period': 'am'}]),
        ('с четверти третьего ждем гостей', [{'hours': 2, 'minutes': 15}]),
        # time of day in the middle
        ('восемь утра пять минут', [{'hours': 8, 'minutes': 5, 'period': 'am'}]),
        ('шесть вечера тридцать', [{'hours': 6, 'minutes': 30, 'period': 'pm'}]),
        ('пять утра пятнадцать минут тридцать секунд', [{'hours': 5, 'minutes': 15, 'seconds': 30, 'period': 'am'}]),
        ('час дня двадцать', [{'hours': 1, 'minutes': 20, 'period': 'pm'}]),
        ('три ночи три минуты', [{'hours': 3, 'minutes': 3, 'period': 'am'}]),
        ('6 утра 6 15', [
            {'hours': 6, 'period': 'am'},
            {'hours': 6, 'minutes': 15}
        ])
    ])
    def test_without_markup(self, parser, samples_extractor, phrase, values):
        results = self(parser, samples_extractor, phrase)
        for i, result in enumerate(results):
            assert result.value == values[i]


class TestNluFstNum(TestNluFst):
    NAME = 'NUM'

    def test_num_currency(self, parser, samples_extractor):
        r = self(parser, samples_extractor, '67 тысяч рублей равны одной тысячи долларов')
        assert r == [
            Entity(start=1, end=2, type='NUM', value=67000),
            Entity(start=4, end=5, type='NUM', value=1000)
        ]

    def test_num_within_datetime(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'номер на 5 человек на 5 декабря 2015')
        assert r == [
            Entity(start=3, end=4, type='NUM', value=5),
            Entity(start=6, end=7, type='NUM', value=5),
            Entity(start=8, end=9, type='NUM', value=2015)
        ]

    def test_num_multitoken(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "хаус 5 сезон 10 серия")
        assert r == [
            Entity(start=2, end=3, type='NUM', value=5),
            Entity(start=4, end=5, type='NUM', value=10)
        ]

    def test_with_abnormal_punkt(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'скачать windows 81 enterprise (x86|x64) matros edition v062015 [2015 rus]')
        assert r == [
            Entity(start=3, end=4, type='NUM', value=81),
            Entity(start=10, end=11, type='NUM', value=2015)
        ]

    def _assert_inflected(self, parser, samples_extractor, text, expected):
        infl = Inflector('ru')
        for gender in GRAM_GENDER:
            for case in GRAM_CASE:
                assert self(parser, samples_extractor, infl.inflect(text, {gender, case}))[0].value == expected

    @pytest.mark.parametrize("number,expected_value", [
        ('первый', 1), ('второй', 2), ('третий', 3)
    ])
    def test_ordinal(self, parser, samples_extractor, number, expected_value):
        self._assert_inflected(parser, samples_extractor, number, expected_value)

    @pytest.mark.parametrize("nouns,expected_value", [
        ('единица', 1), ('двойка', 2), ('тройка', 3),
        ('четверка', 4), ('пятерка', 5), ('шестерка', 6),
        ('семерка', 7), ('восьмерка', 8), ('девятка', 9),
        ('десятка', 10), ('единичка', 1), ('двоечка', 2),
        ('троечка', 3), ('четверочка', 4), ('пятерочка', 5),
        ('шестерочка', 6), ('семерочка', 7), ('восьмерочка', 8),
        ('девяточка', 9), ('десяточка', 10), ('нолик', 0),
        ('зеро', 0), ('zero', 0), ('однерка', 1),
        ('однерочка', 1), ('тройбас', 3),
        ('пятак', 5)
    ])
    def test_nouns(self, parser, samples_extractor, nouns, expected_value):
        self._assert_inflected(parser, samples_extractor, nouns, expected_value)

    def test_zero_negative_cardinal(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'ноль кельвинов это -273 градуса цельсия')
        assert r[0].value == 0
        assert r[1].value == -273

    @pytest.mark.parametrize('input, expected_value', [
        ('громкость минус 2', -2),
        ('громкость -2', -2)
    ])
    def test_num(self, parser, samples_extractor, input, expected_value):
        assert self(parser, samples_extractor, input)[0].value == expected_value


class TestNluFstGeo(TestNluFst):
    NAME = 'GEO'

    @pytest.mark.parametrize("phrase, values", [
        ('по адресу москва ленинградский проспект 6', [{
            'city': {'id': 213, 'name': 'Москва'},
            'street': 'ленинградский',
            'house': 6
        }]),
        ('в Нью-Йорке, на 5-ом авеню', [{
            'city': {'id': 202, 'name': 'Нью-Йорк'}
        }]),

        ('в Нью-Йорке, на 5-ом авеню строения один', [{
            'city': {'id': 202, 'name': 'Нью-Йорк'},
            'street': '5 ом',
            'house': 1
        }]),
        ('в Нью-Йорке, на пятом авеню строения один', [{
            'city': {'id': 202, 'name': 'Нью-Йорк'},
            'street': '5',
            'house': 1
        }]),
        ('лондон - столица великобритании или англии?', [{
            'city': {'id': 103511, 'name': 'Лондон'}
        }, {
            'country': {'id': 102, 'name': 'Великобритания'},
        }, {
            'country': {'id': 102, 'name': 'Великобритания'}
        }]),
        ('Лондон -столица Европы', [{
            'city': {'id': 103511, 'name': 'Лондон'}
        }, {
            'continent': {'id': 111, 'name': 'Европа'}
        }]),
        ('буду в питере', [{
            'city': {'id': 2, 'name': 'Санкт-Петербург'}
        }]),
        ('закажи мне такси от улицы льва толстого дом 16 до одинцово через 15 минут', [{
            'street': 'льва толстого',
            'house': 16
        }, {
            'city': {'id': 10743, 'name': 'Одинцово'}
        }]),
        ('от улицы дом 16 до', [{
            'street': 'дом',
            'house': 16
        }]),
        ('от улицы льва дом 16 до', [{
            'street': 'льва',
            'house': 16
        }]),
        ('от улицы льва николаевича толстого дом 16 до', [{
            'street': 'льва николаевича толстого',
            'house': 16
        }]),
        ('от улицы графа льва николаевича толстого дом 16 до', []),
        ('машину от улицы 8-го марта дом 3', [{
            'street': '8 го марта',
            'house': 3
        }]),
        ('машину от улицы 8 марта дом 3', [{
            'street': '8 марта',
            'house': 3
        }]),
        ('машину от улицы 26 бакинских комиссаров дом 3', [{
            'street': '26 бакинских комиссаров',
            'house': 3
        }]),
        ('улица бухвостова 11к11', [{
            'street': 'бухвостова',
            'house': '11 к11'
        }]),
        ('улица бухвостова 11/11', [{
            'street': 'бухвостова',
            'house': '11 / 11'
        }]),
        ('улица бухвостова 11/11 строение 3 подъезд 6', [{
            'street': 'бухвостова',
            'house': '11 / 11 строение 3 подъезд 6'
        }]),
        ('поезд до новых черемушек', [{
            'metro_station': {
                'id': 20436,
                'name': 'Новые Черемушки'
            }
        }]),
        ('до новослободской', [{
            'metro_station': {
                'id': 20512,
                'name': 'Новослободская'
            }
        }]),
        ('встретимся на пролетарской', [{
            'metro_station': {
                'id': 101939,
                'name': 'Пролетарская'
            }
        }]),
        ('едь на пролетарскую', [{
            'metro_station': {
                'id': 101939,
                'name': 'Пролетарская'
            }
        }]),
        ('ехал с парка культуры', [{
            'metro_station': {
                'id': 101957,
                'name': 'Парк Культуры'
            }
        }]),
    ])
    def test_geo(self, parser, samples_extractor, phrase, values):
        assert [e.value for e in self(parser, samples_extractor, phrase)] == values

    def test_countries_list(self, parser, samples_extractor):
        r = self(parser, samples_extractor,
                 "жители России, США, Соломоновых островов, папуа-"
                 "новой гвинеи, боснии и герцеговины и Шри-Ланки")
        assert r == [
            Entity(start=2, end=3, type='GEO', value={'country': {'id': 225, 'name': 'Россия'}}),
            Entity(start=3, end=4, type='GEO', value={'country': {'id': 84, 'name': 'США'}}),
            Entity(start=4, end=6, type='GEO', value={
                'country': {'id': 20915, 'name': 'Соломоновы острова'}}),
            Entity(start=6, end=8, type='GEO', value={
                'country': {'id': 20739, 'name': 'Папуа-Новая Гвинея'}}),
            Entity(start=8, end=11, type='GEO', value={
                'country': {'id': 10057, 'name': 'Босния и Герцеговина'}}),
            Entity(
                start=12,
                end=13,
                type='GEO',
                value={'country': {'id': 10109, 'name': 'Шри-Ланка'}}
            )
        ]

    def test_city(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "какая погода в дмитрове")
        assert r == [Entity(start=4, end=5, type='GEO', value={'city': {'id': 10723, 'name': 'Дмитров'}})]

    def test_city_para_list(self, parser, samples_extractor):
        r = self(parser, samples_extractor, "погода в спб, мск и нижнем")
        assert r == [
            Entity(start=3, end=4, type='GEO', value={'city': {'id': 2, 'name': 'Санкт-Петербург'}}),
            Entity(start=6, end=7, type='GEO', value={'city': {'id': 47, 'name': 'Нижний Новгород'}})
        ]

    def test_location_from_to(self, parser, samples_extractor):
        assert self(
            parser,
            samples_extractor,
            'такси от улицы Льва Толстого 16 а до Одинцово в четверть шестого вечера'
        ) == [
            Entity(
                start=3,
                end=8,
                type='GEO',
                value={
                    'street': 'льва толстого',
                    'house': '16 а'
                }
            ),
            Entity(
                start=9,
                end=10,
                type='GEO',
                value={
                    'city': {'id': 10743, 'name': 'Одинцово'}
                }
            )
        ]


class TestNluFstPath(TestNluFst):
    NAME = 'PATH'

    @pytest.mark.skip(reason='not implemented')
    def test_strange_aspx(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'https://portal/_layouts/15/start.aspx#/style%20library/default.aspx')
        assert r == [Entity(
            start=1,
            end=2,
            type='PATH',
            value='https://portal/_layouts/15/start.aspx#/style%20library/default.aspx'
        )]

    @pytest.mark.skip(reason='not implemented')
    def test_www_yandex(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'искать на www.yandex.ru')
        assert r == [Entity(
            start=3,
            end=4,
            type='PATH',
            value='www.yandex.ru'
        )]

    @pytest.mark.skip(reason='not implemented')
    def test_yandex(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'искать на yandex.ru')
        assert r == [Entity(
            start=3,
            end=4,
            type='PATH',
            value='yandex.ru'
        )]


class TestPoiRu(TestNluFst):
    def test_poi_category(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'аутсорсинг')
        assert r == [
            Entity(start=1, end=2, type='POI_CATEGORY_RU', value='1839')
        ]


class TestCurrencyRu(TestNluFst):
    @pytest.mark.parametrize("phrase,gold_res", [
        ("сто рублей", Entity(start=2, end=3, type='CURRENCY', value='RUR')),
        ("20 баксов", Entity(start=2, end=3, type='CURRENCY', value='USD')),
        ("1 деноминированный белорусский рубль", Entity(start=2, end=5, type='CURRENCY', value='BYN')),
        ("очень много фунтов стерлингов", Entity(start=3, end=5, type='CURRENCY', value='GBP')),
        ("сколько это в INR ?", Entity(start=4, end=5, type='CURRENCY', value='INR'))
    ])
    def test_currency(self, parser, samples_extractor, phrase, gold_res):
        r = self(parser, samples_extractor, phrase)
        assert gold_res in r, 'Wrong result for utterance {0}, {1} versus {2}'.format(phrase, r, gold_res)


class TestSite(TestNluFst):
    NAME = 'SITE'

    @pytest.mark.parametrize('sample, result', [
        ('открой мне хабрахабр', Entity(start=3, end=4, type='SITE', value='habrahabr.ru')),
        ('открой мне скайп', Entity(start=3, end=4, type='SITE', value='www.skype.com')),
        ('открой вконтакте', Entity(start=2, end=3, type='SITE', value='https://vk.com')),
        ('сайт вконтакте', Entity(start=2, end=3, type='SITE', value='https://vk.com')),
        ('открой яндекс афиша', Entity(start=2, end=4, type='SITE', value='https://afisha.yandex.ru')),
        ('яндекс афиша', Entity(start=1, end=3, type='SITE', value='https://afisha.yandex.ru')),
        ('скачать яндекс браузер', Entity(start=2, end=4, type='SITE', value='https://browser.yandex.ru')),
        ('яндекс браузер', Entity(start=1, end=3, type='SITE', value='https://browser.yandex.ru')),
    ])
    def test_site(self, parser, samples_extractor, sample, result):
        assert self(parser, samples_extractor, sample)[0] == result


class TestSoft(TestNluFst):
    NAME = 'SOFT'

    @pytest.mark.parametrize('sample, result, rules_result', [
        ('открой мне скайп', Entity(start=3, end=4, type='SOFT', value='skype'),
         {"FstSoft": {"entities": [
             "{\"start\":0,\"end\":1,\"type\":\"\",\"substr\":\"<s>\",\"value\":{\"string_value\":\"<s>\"}}",
             "{\"start\":1,\"end\":2,\"type\":\"\",\"substr\":\"открой\",\"value\":{\"string_value\":\"открой\"}}",
             "{\"start\":2,\"end\":3,\"type\":\"\",\"substr\":\"мне\",\"value\":{\"string_value\":\"мне\"}}",
             "{\"start\":3,\"end\":4,\"type\":\"SOFT\",\"substr\":\"скайп\",\"value\":{\"string_value\":\"skype\"}}",
             "{\"start\":4,\"end\":5,\"type\":\"\",\"substr\":\"</s>\",\"value\":{\"string_value\":\"</s>\"}}"]}}),
        ('скачать яндекс браузер', Entity(start=2, end=4, type='SOFT', value='яндекс.браузер'),
         {"FstSoft": {"entities": [
             "{\"start\":0,\"end\":1,\"type\":\"\",\"substr\":\"<s>\",\"value\":{\"string_value\":\"<s>\"}}",
             "{\"start\":1,\"end\":2,\"type\":\"\",\"substr\":\"скачать\",\"value\":{\"string_value\":\"скачать\"}}",
             "{\"start\":2,\"end\":4,\"type\":\"SOFT\",\"substr\":\"яндекс браузер\",\"value\":{\"string_value\":"
             "\"яндекс.браузер\"}}",
             "{\"start\":4,\"end\":5,\"type\":\"\",\"substr\":\"</s>\",\"value\":{\"string_value\":\"</s>\"}}"]}}),
        ('яндекс браузер', Entity(start=1, end=3, type='SOFT', value='яндекс.браузер'),
         {"FstSoft": {"entities": [
             "{\"start\":0,\"end\":1,\"type\":\"\",\"substr\":\"<s>\",\"value\":{\"string_value\":\"<s>\"}}",
             "{\"start\":1,\"end\":3,\"type\":\"SOFT\",\"substr\":\"яндекс браузер\",\"value\":{\"string_value\":"
             "\"яндекс.браузер\"}}",
             "{\"start\":3,\"end\":4,\"type\":\"\",\"substr\":\"</s>\",\"value\":{\"string_value\":\"</s>\"}}"]}})
    ])
    def test_soft(self, parser, samples_extractor, sample, result, rules_result):
        annotations = {'wizard': WizardAnnotation(markup=None, rules=rules_result)}
        assert self(parser, samples_extractor, sample, annotations=annotations)[0] == result


class TestFio(TestNluFst):
    NAME = 'FIO'

    @pytest.mark.parametrize("name", [
        "маша",
        "машу",
        "машей"
    ])
    def test_name(self, parser, samples_extractor, name):
        assert self(parser, samples_extractor, 'просто %s' % name) == [
            Entity(
                start=2,
                end=3,
                type='FIO',
                value={
                    'name': 'маша'
                }
            )]

    @pytest.mark.parametrize("name_surname", [
        "иван иванов",
        "ивану иванову",
        "иванов иван",
        "ивановым иваном",
        "иван,иванов",
        "ивану,иванову",
        "иванов, иван",
        "ивановым, иваном"
    ])
    def test_name_surname(self, parser, samples_extractor, name_surname):
        r = self(parser, samples_extractor, name_surname)[0]
        assert r.start == 1
        assert r.end == 3
        assert r.value['name'] == 'иван'
        # TODO: inflector doesn't care about gender
        assert r.value['surname'] in ('иванов', 'иванова')

    @pytest.mark.parametrize("fio", [
        "иванов иван иванович",
        "петром петровичем петровым",
        "владимировым владимиром владимировичем",
        'александровой александрой александровной',
        'евгеньева евгения евгенична',
    ])
    def test_fio_full(self, parser, samples_extractor, fio):
        r = self(parser, samples_extractor, 'меня зовут %s' % fio)[0]
        assert r.start == 3
        assert r.end == 6
        assert all(k in r.value for k in ('name', 'surname', 'patronym'))

    @pytest.mark.parametrize("surname_ii", [
        "путин В.В.",
        "Путин В. В.",
        "в.в. путин",
        "в. в. путин",
        "путин в в",
        "в в путин"
    ])
    def test_surname_initials(self, parser, samples_extractor, surname_ii):
        r = self(parser, samples_extractor, "президент россии %s" % surname_ii)[0]
        assert r.start == 3
        assert r.end in (5, 6)
        assert r.value['name'] == 'в'
        assert r.value['patronym'] == 'в'
        assert r.value['surname'].startswith('путин')

    @pytest.mark.skip(reason='Not implemented: pymorphy incorrectly inflects "лев"')
    def test_in_geo(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'улица льва толстого 16')[0]
        assert r.value == {
            'name': 'лев',
            'surname': 'толстой'
        }

    @pytest.mark.parametrize('name,expected_result', [
        ('александром', 'александр'),  # a competing hypothesis is александръ
        ('сергею', 'сергей'),  # a competing hypothesis is сергёй
        ('ольги', 'ольга'),  # a competing hypothesis is ольг
        ('алексее', 'алексей'),  # a competing hypothesis is алёксей
        ('юлией', 'юлия'),  # a competing hypothesis is юлія
        ('лене', 'лена'),  # a competing hypothesis is лёня
        ('лёне', 'лёня'),  # but the word лёня must be interpreted properly
        ('ане', 'аня'),  # a competing hypothesis is ани
        ('владу', 'влад'),  # a competing hypothesis is влада
        ('павлом', 'павел'),  # a competing hypothesis is павла
        ('мариной', 'марина'),  # a competing hypothesis is марине
    ])
    def test_first_name_resolution(self, parser, samples_extractor, name, expected_result):
        # we need ru_nonorm for лёня, because otherwise ё is replaced with е.
        r = self(parser, samples_extractor, name, samples_extractor_key='ru_nonorm')[0]
        assert r.value['name'] == expected_result


class TestDatetimeRange(TestNluFst):
    NAME = 'DATETIME_RANGE'

    @pytest.mark.parametrize('utts', [
        'погода с 15 по 25 декабря',
        'погода с 15 декабря по 25 декабря в москве',
        'погода от 15 до 25 декабря'
    ])
    def test_from_to_date(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'months': 12, 'days': 15},
            'end': {'months': 12, 'days': 25}
        }

    @pytest.mark.parametrize('utts', [
        'погода с 15 по 25',
        'погода с 15 по 25 число',
        'погода с пятнадцатого по двадцать пятое',
        'погода с пятнадцатого по двадцать пятое число',
    ])
    def test_from_to_date_no_month(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'days': 15},
            'end': {'days': 25}
        }

    def test_from_to_date_different_months(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'погода с 10 ноября по 15 декабря')[0].value == {
            'start': {'months': 11, 'days': 10},
            'end': {'months': 12, 'days': 15}
        }

    def test_from_to_date_different_months_and_year(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'погода с 25 декабря до 10 января')[0].value == {
            'start': {'months': 12, 'days': 25},
            'end': {'months': 1, 'days': 10, 'years': 1, 'years_relative': True}
        }

    def test_on_next_n_days(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'погода на 3 дня')[0]
        assert r.start == 3
        assert r.end == 5
        assert r.value == {
            'start': {'days_relative': True, 'days': 0},
            'end': {'days_relative': True, 'days': 3}
        }

    def test_on_next_time(self, parser, samples_extractor):
        r = self(parser, samples_extractor, 'расписание на следующие 3 часа')[0]
        assert r.start == 3
        assert r.end == 6
        assert r.value == {
            'start': {'hours': 0, 'hours_relative': True},
            'end': {'hours': 3, 'hours_relative': True}
        }

    def test_on_next_n_combined(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'погода на ближайшие полторы недели')[0].value == {
            'start': {'days': 0, 'days_relative': True, 'weeks': 0, 'weeks_relative': True},
            'end': {'days': 3, 'days_relative': True, 'weeks': 1, 'weeks_relative': True}
        }

    @pytest.mark.parametrize('utts', [
        'погода на день',
        'погода на один день',
        'погода на этот день'
    ])
    def test_on_next_day(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'days_relative': True, 'days': 0},
            'end': {'days_relative': True, 'days': 1}
        }

    @pytest.mark.parametrize('utts', [
        'погода на неделю',
        'погода на одну неделю',
        'погода на эту неделю'
    ])
    def test_on_current_week(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'weeks_relative': True, 'weeks': 0},
            'end': {'weeks_relative': True, 'weeks': 1}
        }

    @pytest.mark.parametrize('utts', [
        'погода на следующую неделю',
        'погода на следующей неделе',
        'погода на ту неделю'
    ])
    def test_on_next_week(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'weeks_relative': True, 'weeks': 1},
            'end': {'weeks_relative': True, 'weeks': 2}
        }

    @pytest.mark.parametrize('utts', [
        'погода на сегодня-завтра',
        'погода на сегодня завтра',
        'погода на сегодня и завтра',
        'погода на сегодня и на завтра',
        'погода на ближайшие пару дней'
    ])
    def test_on_today_tomorrow(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'days_relative': True, 'days': 0},
            'end': {'days_relative': True, 'days': 2}
        }

    @pytest.mark.parametrize('utts', [
        'погода на сегодня, завтра и послезавтра',
        'погода на сегодня, на завтра и на послезавтра',
        'погода на сегодня завтра послезавтра',
    ])
    def test_on_today_tomorrow_after_tomorrow(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'days_relative': True, 'days': 0},
            'end': {'days_relative': True, 'days': 3}
        }

    @pytest.mark.parametrize('utts', [
        'погода на выходных',
        'погода в выходные',
        'погода на этих выходных',
        'погода на уикенд',
        'погода на викенде'
    ])
    def test_on_weekend(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'weekend': True, 'weeks': 0, 'weeks_relative': True},
            'end': {'weekend': True, 'weeks': 0, 'weeks_relative': True}
        }

    @pytest.mark.parametrize('utts', [
        'погода на следующих выходных',
        'погода в следующие выходные',
        'погода на следующий уикенд',
        'погода на следующем викенде'
    ])
    def test_next_weekend(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'weekend': True, 'weeks': 1, 'weeks_relative': True},
            'end': {'weekend': True, 'weeks': 1, 'weeks_relative': True}
        }

    @pytest.mark.parametrize('utts', [
        'погода на праздниках',
        'погода на ближайших праздниках',
        'погода в ближайшие праздники'
    ])
    def test_on_holidays(self, parser, samples_extractor, utts):
        r = self(parser, samples_extractor, utts)[0]
        assert r.start == 3
        assert r.value == {
            'start': {'holidays': True},
            'end': {'holidays': True}
        }

    @pytest.mark.parametrize('utts', [
        'погода с четверга по субботу',
        'погода с этого четверга по субботу',
        'погода с этого четверга по ближайшую субботу',
    ])
    def test_from_to_weekday(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'weekday': 4},
            'end': {'weekday': 6}
        }

    @pytest.mark.parametrize('utts', [
        'погода с четверга по понедельник',
        'погода с четверга по следующий понедельник'
    ])
    def test_from_to_weekday_next_week(self, parser, samples_extractor, utts):
        assert self(parser, samples_extractor, utts)[0].value == {
            'start': {'weekday': 4},
            'end': {'weekday': 1, 'weeks': 1, 'weeks_relative': True}
        }

    def test_up_to_weekday(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'погода до пятницы')[0].value == {
            'start': {'days_relative': True, 'days': 0},
            'end': {'weekday': 5}
        }

    @pytest.mark.parametrize('inputs, outputs', [
        ('погода со вторника по среду', {'start': {'weekday': 2}, 'end': {'weekday': 3}}),
        ('погода со среды по четверг', {'start': {'weekday': 3}, 'end': {'weekday': 4}})
    ])
    def test_weekday_range(self, parser, samples_extractor, inputs, outputs):
        assert self(parser, samples_extractor, inputs)[0].value == outputs

    @pytest.mark.parametrize("inputs, outputs", [
        ('погода на 15 16 мая', {'start': {'days': 15, 'months': 5}, 'end': {'days': 16, 'months': 5}}),
        ('погода на 15 20 мая', {'start': {'days': 15, 'months': 5}, 'end': {'days': 20, 'months': 5}}),
        ('погода на 15-16 мая', {'start': {'days': 15, 'months': 5}, 'end': {'days': 16, 'months': 5}}),
        ('погода на 15 - 16 мая', {'start': {'days': 15, 'months': 5}, 'end': {'days': 16, 'months': 5}}),
        ('погода на 15 и 16 мая', {'start': {'days': 15, 'months': 5}, 'end': {'days': 16, 'months': 5}}),
        ('погода на 15 35 мая', []),
    ])
    def test_from_to_monthdays(self, parser, samples_extractor, inputs, outputs):
        result = self(parser, samples_extractor, inputs)
        if result:
            result = result[0].value
        assert result == outputs


class TestFloat(TestNluFst):

    NAME = 'FLOAT'

    @pytest.mark.parametrize("utt, output", [
        ('пи равно три целых четырнадцать сотых', 3.14),
        ('пи равно 3 целых четырнадцать сотых', 3.14),
        ('пи равно три целых 14 сотых', 3.14),
        ('пи равно 3.14', 3.14),
        pytest.param('1 миллионная доллара в рублях', 0.000001, marks=pytest.mark.xfail(reason='Unable to denormalize partially literated numbers')),
        ('одна миллионная доллара в рублях', 0.000001),
        ('поставь волну сто и пять', 100.5)
    ])
    def test_float(self, parser, samples_extractor, utt, output):
        assert self(parser, samples_extractor, utt)[0].value == output

    @pytest.mark.parametrize("utt", [
        'пи равно трем целым четырнадцати сотым',
    ])
    def test_float_inflected(self, parser, samples_extractor, utt):
        assert self(parser, samples_extractor, utt)[0].value == 3.14

    @pytest.mark.parametrize("utt", [
        'пять сотых рублей'
    ])
    def test_fraction(self, parser, samples_extractor, utt):
        assert self(parser, samples_extractor, utt)[0].value == 0.05

    @pytest.mark.parametrize("utt", [
        'пятью сотыми рублями',
    ])
    def test_fraction_inflected(self, parser, samples_extractor, utt):
        assert self(parser, samples_extractor, utt)[0].value == 0.05

    def test_num_and_quarter(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'пять с четвертью рублей')[0].value == 5.25

    def test_num_and_half(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'пять с половиной рублей')[0].value == 5.5

    def test_int_frac_parts(self, parser, samples_extractor):
        assert self(parser, samples_extractor, 'с вас три пятьдесят')[0].value == 3.50

    def test_zero_negative(self, parser, samples_extractor):
        r = self(parser, samples_extractor, '0,0 кельвинов это -273.15 градуса цельсия')
        assert r[0].value == 0.0
        assert r[1].value == -273.15

    @pytest.mark.parametrize('utt,norm', (
        ('49/0 5', 'ru'),
        ('49/0 5', 'ru_nonorm'),
        ('49 / 0 5', 'ru'),
        ('заинск проспект победы 1/0 6 а', 'ru'),
        ('заинск проспект победы 1/0 6 а', 'ru_nonorm'),
        ('разбуди меня в шесть ноль ноль', 'ru'),
        ('напомни мне завтра в 3 00 взять с собой лестницу', 'ru')
    ))
    def test_fractions_division_by_zero(self, parser, samples_extractor, utt, norm):
        assert self(parser, samples_extractor, utt, norm) == []


class TestCalc(TestNluFst):

    NAME = 'CALC'

    @pytest.mark.parametrize("utt, output", [
        ('69 умножить на 3', {'start': 1, 'end': 5}),
        ('посчитай пи разделить на 2', {'start': 2, 'end': 6}),
        ('2 + 2', {'start': 1, 'end': 4}),
        ('2/2', {'start': 1, 'end': 4}),
        ('вычислить 0.05 + ctan(3.65) / 5.51 на калькуляторе', {'start': 2, 'end': 8}),
        ('cos(0.29)', {'start': 1, 'end': 3}),
        ('сколько будет cos(0.29)', {'start': 3, 'end': 5}),
        ('cos(0.29) это сколько', {'start': 1, 'end': 3}),
        ('арккосинус шесть восьмых сколько будет', {'start': 1, 'end': 3}),
        ('100 умножить на 2 разделить на 8', {'start': 1, 'end': 8}),
        ('раздели 1003 на 7', {'start': 1, 'end': 5}),
        ('к 22 прибавь 16', {'start': 1, 'end': 5}),
        ('пи пополам делить на тысяча десять плюс синус тринадцать', {'start': 1, 'end': 9}),
        ('минус пять на пи пополам', {'start': 1, 'end': 6}),
        ('косинус шесть восьмых умножить синус сто пять', {'start': 1, 'end': 6}),
        ('трижды девять', {'start': 1, 'end': 3}),
        ('трижды девять прибавить семь четвертых', {'start': 1, 'end': 5}),
    ])
    def test_calc(self, parser, samples_extractor, utt, output):
        r = self(parser, samples_extractor, utt)[0]
        assert r.start == output['start']
        assert r.end == output['end']


class TestUnitsTime(TestNluFst):

    NAME = 'UNITS_TIME'

    @pytest.mark.parametrize('input, output_value', [
        ('минута', {'minutes': 1}),
        ('через минуту', {'minutes': 1}),
        ('час', {'hours': 1}),
        ('полминуты', {'minutes': 0.5}),
        ('полторы минуты', {'minutes': 1.5}),
        ('сто двадцать три минуты', {'minutes': 123}),
        ('сто минут сорок секунд', {'minutes': 100, 'seconds': 40}),
        ('сто минут и сорок секунд', {'minutes': 100, 'seconds': 40}),
        ('на пару минут', {'minutes': 2}),
        ('на 5 30', {'minutes': 5, 'seconds': 30}),
        ('на 120', {'minutes': 120}),
        ('вырази в минутах 23/25 часа', {'hours': 0.92}),
        ('перемотай на час двадцать', {'hours': 1, 'minutes': 20}),
        ('засеки минуту сорок', {'minutes': 1, 'seconds': 40}),
    ])
    def test_units_time(self, parser, samples_extractor, input, output_value):
        assert self(parser, samples_extractor, input)[0].value == output_value


class TestWeekday(TestNluFst):

    NAME = 'WEEKDAYS'

    @pytest.mark.parametrize('utt, output, start, end', [
        ('поставь будильник в понедельник на 7 часов', {'weekdays': [1], 'repeat': False}, 3, 5),
        ('поставь будильник с понедельника по среду на 7 часов', {'weekdays': [1, 2, 3], 'repeat': False}, 3, 7),
        ('поставь будильник по пятницам на 7 часов', {'weekdays': [5], 'repeat': True}, 3, 5),
        ('поставь будильник на четверг на 7 часов', {'weekdays': [4], 'repeat': False}, 3, 5),
        ('на будни поставь будильник в 10', {'weekdays': [1, 2, 3, 4, 5], 'repeat': False}, 1, 3),
        ('разбуди в 10 по будням', {'weekdays': [1, 2, 3, 4, 5], 'repeat': True}, 4, 6),
        ('по выходным будильник на 10', {'weekdays': [6, 7], 'repeat': True}, 1, 3),
        ('в выходные будет хорошая погода', {'weekdays': [6, 7], 'repeat': False}, 1, 3),
        ('поставь будильники на вторники на 7 часов', {'weekdays': [2], 'repeat': True}, 3, 5),
        ('в понедельник и вторник напомни зайти к врачу', {'weekdays': [1, 2], 'repeat': False}, 1, 5),
        ('в понедельник четверг и вторник разбуди в 8', {'weekdays': [1, 2, 4], 'repeat': False}, 1, 6),
        ('по понедельникам средам и пятницам у меня английский', {'weekdays': [1, 3, 5], 'repeat': True}, 1, 6),
        ('каждое воскресенье я хожу в баню', {'weekdays': [7], 'repeat': True}, 1, 3),
        ('каждые выходные напоминай проснуться', {'weekdays': [6, 7], 'repeat': True}, 1, 3),
        ('с понедельника начинаю новую жизнь', {'weekdays': [1, 2, 3, 4, 5, 6, 7], 'repeat': False}, 1, 3),
        ('с пятницы по вторник я буду недоступен', {'weekdays': [1, 2, 5, 6, 7], 'repeat': False}, 1, 5),
        ('я буду недоступен со среды по среду', {'weekdays': [1, 2, 3, 4, 5, 6, 7], 'repeat': False}, 4, 8),
        ('каждый день в семь часов', {'weekdays': [1, 2, 3, 4, 5, 6, 7], 'repeat': True}, 1, 3),
    ])
    def test(self, parser, samples_extractor, utt, output, start, end):
        result = self(parser, samples_extractor, utt)[0]
        assert result.value == output
        assert result.start == start
        assert result.end == end


class TestSwear(TestNluFst):

    NAME = 'SWEAR'

    @pytest.mark.parametrize('utt, output', [
        ('это пиздец', {'start': 2, 'end': 3}),
        ('блядь блядь нет', {'start': 1, 'end': 2}),
        ('хуй пизда', {'start': 1, 'end': 2}),
    ])
    def test_swear(self, parser, samples_extractor, utt, output):
        r = self(parser, samples_extractor, utt)[0]
        assert r.start == output['start']
        assert r.end == output['end']
