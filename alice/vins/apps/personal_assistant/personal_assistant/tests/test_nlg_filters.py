# coding: utf-8
from __future__ import unicode_literals

import pytest
import requests_mock
from freezegun import freeze_time

from vins_core.utils.data import open_resource_file

from personal_assistant.nlg_filters import (
    pluralize_tag,
    inflect_amount_of_money,
    try_round_float,
    split_cost_with_spaces,
    ceil_seconds,
    alarm_time_format,
    music_title_shorten,
    image_ratio,
    render_date_with_on_preposition,
)


@pytest.mark.parametrize('string, res', [
    ('', ''),
    ('#meter', '#meter'),
    ('test', 'test'),
    ('123 #word', '123 #word'),

    ('масса 1 #kilogram', 'масса 1 килограмм'),
    ('высота 2 #meter', 'высота 2 метра'),
    ('длина 3 #centimeter', 'длина 3 сантиметра'),
    ('вес 4 #gram', 'вес 4 грамма'),
    ('продолжительность 5 #minute', 'продолжительность 5 минут'),
    ('площадь 6 #square_kilometer', 'площадь 6 квадратных километров'),

    ('вес 9 532 #gram', 'вес 9 532 грамма'),
    ('вес 1 900 532 #gram', 'вес 1 900 532 грамма'),

    ('0 #meter', '0 метров'),
    ('1 #meter', '1 метр'),
    ('2 #meter', '2 метра'),
    ('3 #meter', '3 метра'),
    ('4 #meter', '4 метра'),
    ('5 #meter', '5 метров'),
    ('6 #meter', '6 метров'),
    ('22 #meter', '22 метра'),
])
def test_pluralize_tag(string, res):
    assert pluralize_tag(string) == res


@pytest.mark.parametrize('amount, currency, sub_currency, case, sub_currency_digit_capacity, res', [
    (10.5, 'российский рубль', 'копейка', 'dat', 2, '10 российским рублям 50 копейкам'),
    (6001.448, 'доллар', 'цент', 'dat', 2, '6001 доллару 45 центам'),
    (0.448, 'доллар', 'цент', 'dat', 2, '45 центам'),
    (12, 'доллар', None, 'acc', 2, '12 долларов'),
    (11, 'доллар', None, 'dat', 2, '11 долларам'),
    (1000000.454678, 'имперский кредит', 'имперская копейка', 'acc', 3,
     '1000000 имперских кредитов 455 имперских копеек'),
    (6000.00, 'доллар', 'цент', 'abl', 7, '6000 долларов'),
    (6075.00, 'доллар', 'цент', 'abl', 7, '6075 долларах'),
    (0.678, 'белорусский рубль', None, 'ins', 2, '0.68 белорусского рубля'),
    (725.045, 'рубль', 'копейка', 'dat', 2, '725 рублям 4 копейкам'),
    (9.278, 'евро', 'евроцент', 'acc', 2, '9 евро 28 евроцентов'),
    (56.31, 'рубль', 'копейка', 'nom', 2, '56 рублей 31 копейка'),
    (0.31, 'рубль', 'копейка', 'nom', 2, '31 копейка'),
])
def test_inflect_amount_of_money(amount, currency, sub_currency, case, sub_currency_digit_capacity, res):
    assert inflect_amount_of_money(amount, currency, sub_currency, case, sub_currency_digit_capacity) == res


@pytest.mark.parametrize('amount, currency, sub_currency, case, sub_currency_digit_capacity, res', [
    (56.31, 'рубль', 'копейка', 'acc', 2, '#acc 56 рублей #acc 31 копейку'),
    (1.00, 'рубль', 'копейка', 'abl', 2, '#loc 1 рубле'),
    (0.31, 'рубль', 'копейка', 'ins', 2, '#instr 31 копейкой'),
])
def test_inflect_amount_of_money_speech_hints(amount, currency, sub_currency, case, sub_currency_digit_capacity, res):
    assert inflect_amount_of_money(
        amount, currency, sub_currency,
        case, sub_currency_digit_capacity,
        speech_hints=True
    ) == res


@pytest.mark.parametrize('str, res', [
    ('56.31', '56.31'),
    ('1', '1'),
    ('100000000', '100000000'),
    ('1567.6464', '1567.6464'),
    ('1567.64648', 'примерно 1567.6465'),
    ('0.08715574275', 'примерно 0.0872'),
    ('1567.6464889475875489', 'примерно 1567.6465'),
    ('not a number', 'not a number'),
    ('', ''),
])
def test_try_round_float(str, res):
    assert try_round_float(str) == res


@pytest.mark.parametrize('cost, result', [
    ('1.5', '1.5'),
    ('150', '150'),
    ('100500', '100 500'),
    ('100500.5', '100 500.5'),
    ('10000000000', '10 000 000 000'),
])
def test_split_cost_with_spaces(cost, result):
    assert split_cost_with_spaces(cost) == result


@pytest.mark.parametrize('units, result', [
    ({'hours': 1, 'minutes': 20, 'seconds': 30}, {'hours': 1, 'minutes': 21}),
    ({'hours': 1, 'minutes': 20}, {'hours': 1, 'minutes': 20}),
    ({'minutes': 20, 'seconds': 30}, {'minutes': 20, 'seconds': 30}),
    ({'seconds': 30}, {'seconds': 30}),
    ({'minutes': 59}, {'minutes': 59}),
    ({'minutes': 20, 'seconds': 30, 'aggressive': True}, {'minutes': 21}),
    ({'minutes': 59, 'seconds': 30, 'aggressive': True}, {'hours': 1}),
    ({'seconds': 30, 'aggressive': True}, {'minutes': 1}),
])
def test_ceil_seconds(units, result):
    assert ceil_seconds(units, aggressive=units.get('aggressive', False)) == result


@pytest.mark.parametrize('time, case, text, voice', [
    ({'hours': 0}, 'nom', '12 часов ночи', '#nom 12 часов ночи'),
    ({'hours': 0}, 'gen', '12 часов ночи', '#gen 12 часов ночи'),
    ({'hours': 0}, 'loc', '12 часах ночи', '#loc 12 часах ночи'),
    ({'hours': 0}, 'ins', '12 часами ночи', '#instr 12 часами ночи'),
    ({'hours': 1}, 'nom', 'час ночи', 'час ночи'),
    ({'hours': 1}, 'dat', 'часу ночи', 'часу ночи'),
    ({'hours': 2}, 'nom', '2 часа ночи', '#nom 2 часа ночи'),
    ({'hours': 7}, 'nom', '7 часов утра', '#nom 7 часов утра'),
    ({'hours': 9, 'period': 'am'}, 'acc', '9 часов утра', '#acc 9 часов утра'),
    ({'hours': 12}, 'nom', '12 часов дня', '#nom 12 часов дня'),
    ({'hours': 12, 'period': 'am'}, 'nom', '12 часов ночи', '#nom 12 часов ночи'),
    ({'hours': 12, 'period': 'pm'}, 'nom', '12 часов дня', '#nom 12 часов дня'),
    ({'hours': 13}, 'nom', '13 часов', '#nom 13 часов'),
    ({'hours': 1, 'period': 'pm'}, 'dat', '13 часам', '#dat 13 часам'),
    ({'hours': 2, 'minutes': 30, 'period': 'pm'}, 'dat', '14:30', '#dat 14 часам #dat 30 минутам'),
    ({'hours': 7, 'minutes': 8}, 'nom', '07:08', '#nom 7 часов #nom 8 минут'),
    ({'hours': 0, 'minutes': 15}, 'nom', '00:15', '#nom 0 часов #nom 15 минут'),
])
def test_alarm_time(time, case, text, voice):
    result = alarm_time_format(time, case=case)
    assert result.text == text
    assert result.voice == voice


@pytest.mark.parametrize('text, short_text', [
    ('just text', 'just text'),
    (' (just text) ', '(just text)'),
    (' , just text', ', just text'),
    (' ; just text', '; just text'),
    ('text (with something in parentheses)', 'text'),
    ('text, with something after comma', 'text'),
    ('text; with something after semicolon', 'text'),
    ('long text, with something after comma', 'long text'),
    ('long text, with something after comma, and a comma', 'long text'),
    ('text (with something in parentheses); and something after semicolon', 'text'),
    ('text (with something in parentheses, and a comma) and something else', 'text  and something else'),
])
def test_nlg_music_title_shorten(text, short_text):
    assert music_title_shorten(text) == short_text


@pytest.mark.parametrize('img_path, ratio', [
    ('personal_assistant/tests/data/images/img_100_100.jpg', 1.),
    ('personal_assistant/tests/data/images/img_200_100.jpg', 2.),
    ('personal_assistant/tests/data/images/img_100_60.jpg', 0.6)
])
def test_image_ratio(img_path, ratio):
    with requests_mock.Mocker() as req_mock:
        with open_resource_file(img_path, encoding=None) as f:
            req_mock.get('http://test.com/img', body=f)
            assert image_ratio('http://test.com/img') == ratio


@freeze_time('2018-01-25 19:45:00')
@pytest.mark.parametrize('dt, phrase, voice_phrase', [
    ({'days': 0, 'days_relative': True}, 'на сегодня', 'на сегодня'),
    ({'days': 1, 'days_relative': True}, 'на завтра', 'на завтра'),
    ({'days': -1, 'days_relative': True}, 'на вчера', 'на вчера'),
    ({'days': -2, 'days_relative': True}, 'на позавчера', 'на позавчера'),
    ({'days': 2, 'days_relative': True}, 'на послезавтра', 'на послезавтра'),
    ({'years': 2018, 'months': 3, 'days': 1}, 'на 1 марта', 'на первое марта'),
    ({'years': 2018, 'months': 3, 'days': 3}, 'на 3 марта', 'на третье марта'),
    ({'years': 2018, 'months': 5, 'days': 8}, 'на 8 мая', 'на восьмое мая'),
    ({'months': 3, 'days': 10}, 'на 10 марта', 'на десятое марта'),
    ({'months': 12, 'days': 6}, 'на 6 декабря', 'на шестое декабря'),
    ({'months': 11, 'days': 12}, 'на 12 ноября', 'на двенадцатое ноября'),
    ({'years': 2018, 'months': 4, 'days': 17}, 'на 17 апреля', 'на семнадцатое апреля'),
    ({'years': 2019, 'months': 4, 'days': 17}, 'на 17 апреля 2019 года', 'на семнадцатое апреля две тысячи девятнадцатого года'),  # noqa
    ({'years': 2020, 'months': 4, 'days': 12}, 'на 12 апреля 2020 года', 'на двенадцатое апреля две тысячи двадцатого года'),  # noqa
    ({'months': 9}, 'на сентябрь', 'на сентябрь'),
    ({'months': 12}, 'на декабрь', 'на декабрь'),
    ({'years': 2021, 'months': 9}, 'на сентябрь 2021 года', 'на сентябрь две тысячи двадцать первого года'),
    ({'years': 2019, 'months': 1}, 'на январь 2019 года', 'на январь две тысячи девятнадцатого года'),
    ({'years': 2028, 'months': 6}, 'на июнь 2028 года', 'на июнь две тысячи двадцать восьмого года'),
    ({'weekday': 3}, 'на среду', 'на среду'),
    ({'weekday': 6}, 'на субботу', 'на субботу'),
    ({'weekday': 6, 'weeks': 1, 'weeks_relative': True}, 'на следующую субботу', 'на следующую субботу'),
    ({'weekday': 1, 'weeks': 1, 'weeks_relative': True}, 'на следующий понедельник', 'на следующий понедельник'),
    ({'weekday': 2, 'weeks': -1, 'weeks_relative': True}, 'на прошлый вторник', 'на прошлый вторник'),
    ({'weekday': 7, 'weeks': -1, 'weeks_relative': True}, 'на прошлое воскресенье', 'на прошлое воскресенье'),
    ({'years': 2018}, 'на 2018 год', 'на две тысячи восемнадцатый год'),
    ({'years': 2023}, 'на 2023 год', 'на две тысячи двадцать третий год'),
    ({'years': 2038}, 'на 2038 год', 'на две тысячи тридцать восьмой год'),
    ({'years': 2040}, 'на 2040 год', 'на две тысячи сороковой год'),
])
def test_render_date_with_on_preposition(dt, phrase, voice_phrase):
    res = render_date_with_on_preposition(dt)
    assert res.text == phrase
    assert res.voice == voice_phrase
