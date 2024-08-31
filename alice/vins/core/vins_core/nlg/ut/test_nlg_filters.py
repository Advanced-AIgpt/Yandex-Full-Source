# coding: utf-8
from __future__ import unicode_literals

import json
import random
from datetime import datetime, timedelta

import pytest
import pytz
from freezegun import freeze_time
from dateutil.parser import parse

from vins_core.utils.datetime import parse_tz

from vins_core.utils.data import load_data_from_file

from vins_core.nlg.filters import (
    choose_tense, human_day_rel, human_date, human_month, human_seconds, human_meters, human_time,
    geo_city_address, city_prepcase, inflect,
    format_datetime, format_weekday, capitalize_first, split_long_numbers,
    singularize, pluralize, shuffle, join, emojize, render_datetime_raw,
    get_item, hostname, to_json, html_escape, render_units_time,
    render_weekday_type, render_short_weekday_name,
    human_time_raw_text_and_voice, tts_domain,
    number_of_readable_tokens, human_age, trim_with_ellipsis
)
from vins_core.nlg.tests import relative_datetime_raw


now = datetime(2017, 1, 25, 19, 45, 00, tzinfo=pytz.UTC)
moscow = parse_tz('Europe/Moscow')


@freeze_time('2017-01-25 19:45:00')
@pytest.mark.parametrize("date, consider_time, future_as_present, result", [
    (now, True, False, 0),
    (now - timedelta(minutes=1), True, False, -1),
    (now + timedelta(minutes=1), True, False, 1),
    (now - timedelta(days=1), True, False, -1),
    (now + timedelta(days=1), True, False, 1),
    (now - timedelta(minutes=1), False, False, 0),
    (now + timedelta(minutes=1), False, False, 0),
    (now - timedelta(hours=1), False, False, 0),
    (now + timedelta(hours=1), False, False, 0),
    (now - timedelta(days=1), False, False, -1),
    (now + timedelta(days=1), False, False, 1),

    (now + timedelta(minutes=1), True, True, 0),
    (now + timedelta(days=1), True, True, 0),
    (now + timedelta(days=1), False, True, 0),
])
def test_choose_tense(date, consider_time, future_as_present, result):
    assert choose_tense(date, consider_time, future_as_present) == result


@freeze_time('2017-01-25 19:44:00')
@pytest.mark.parametrize("delta, result", [
    (timedelta(days=1), 'завтра'),
    (timedelta(hours=10), 'завтра'),
    (timedelta(days=1, hours=4), 'завтра'),

    (timedelta(days=2), 'послезавтра'),

    (timedelta(days=0), 'сегодня'),
    (timedelta(hours=4), 'сегодня'),

    (timedelta(days=-1), 'вчера'),
    (timedelta(days=-2), 'позавчера'),

    (timedelta(days=-3), '22 января'),
    (timedelta(days=3), '28 января'),
    (timedelta(days=365), '25 января 2018 года'),
    (timedelta(days=-365), '26 января 2016 года'),
])
def test_human_day_rel(delta, result):
    assert human_day_rel(now + delta) == result


@freeze_time('2017-01-26 22:44:00')
@pytest.mark.parametrize("date, tz, result", [
    ('2017-01-27 01:43', moscow, 'сегодня'),
    ('2017-01-28 01:44', moscow, 'завтра'),
    ('2017-01-26 22:44', pytz.utc, 'сегодня'),
    ('2017-01-27 22:44', pytz.utc, 'завтра'),
    ('2017-01-26 22:44', 'America/Los_Angeles', 'сегодня'),
    ('2017-01-26 01:44', 'America/Los_Angeles', 'сегодня'),
    ('2017-01-26 20:13', 'Asia/Magadan', 'вчера'),

    ('2017-01-26', pytz.utc, 'сегодня'),
    ('2017-01-26', moscow, 'вчера'),
    ('2017-01-26', 'America/Los_Angeles', 'сегодня'),
    ('2017-01-26', 'Asia/Magadan', 'вчера'),

    ('2017-01-27', pytz.utc, 'завтра'),
    ('2017-01-27', moscow, 'сегодня'),
    ('2017-01-27', 'America/Los_Angeles', 'завтра'),
    ('2017-01-27', 'Asia/Magadan', 'сегодня'),
])
def test_human_day_rel_with_tz(date, tz, result):
    """ Get `date` and should say "today" "tomorrow" or "aftertomorrow"
    with respect to `tz` and current time
    """
    assert human_day_rel(parse(date), tz) == result


@freeze_time('2017-01-25 22:44:00')
@pytest.mark.parametrize('dt, result', [
    (now + timedelta(days=365), '25 января 2018 года'),
    (now, '25 января'),
])
def test_human_date(dt, result):
    assert human_date(dt) == result


@pytest.mark.parametrize('time, case, result', [
    ('00:00', None, 'полночь'),
    ('00:00', 'gen', 'полуночи'),
    ('01:30', None, '1:30 ночи'),
    ('01:30', 'nomn', '1:30 ночи'),
    ('01:30', 'acc', '1:30 ночи'),
    ('01:30', 'accs', '1:30 ночи'),
    ('01:00', None, '1 час ночи'),
    ('01:00', 'gen', '1 часа ночи'),
    ('13:00', None, '1 час дня'),
    ('13:00', 'gen', '1 часа дня'),
    ('15:00', None, '3 часа дня'),
    ('15:00', 'gen', '3 часов дня')
])
def test_human_time(time, case, result):
    assert human_time(parse(time), case=case) == result


@pytest.mark.parametrize('time, case, text, voice', [
    ({'hours': 10, 'minutes': 20}, None, '10:20', '#nomn 10:20'),
    ({'hours': 15, 'minutes': 31}, 'acc', '15:31', '#acc 15:31'),
    ({'hours': 10}, 'acc', '10:00', '#acc 10 утра'),
    ({'hours': 13}, 'nom', '13:00', '#nom 1 час дня'),
    ({'hours': 14}, 'acc', '14:00', '#acc 2 часа дня'),
    ({'hours': 19}, 'nom', '19:00', '#nom 7 вечера'),
    ({'hours': 12, 'minutes': 30}, 'nom', '12:30', '#nom 12:30 дня'),
    ({'hours': 0, 'minutes': 0}, 'nom', '0:00', 'полночь'),
    ({'hours': 3}, 'acc', '3:00', '#acc 3 ночи'),
])
def test_human_time_raw_text_and_voice(time, case, text, voice):
    render_result = human_time_raw_text_and_voice(time, case=case)
    assert render_result.text == text, _error_message('Text', time, text, render_result.text)
    assert render_result.voice == voice, _error_message('Voice', time, voice, render_result.voice)


@freeze_time('2017-01-26 22:44:00')
@pytest.mark.parametrize("date, tz, result", [
    ('2017-01-27 01:43:00 +5', moscow, '26 января'),
    ('2017-01-27 01:43:00 +3', moscow, '27 января'),
    ('2016-01-26 22:44:00 +0', pytz.utc, '26 января 2016 года'),
    ('2016-01-27 22:44:00 +0', moscow, '28 января 2016 года'),
])
def test_human_date_with_tz(date, tz, result):
    assert human_date(parse(date), tz) == result


@freeze_time('2017-01-26 22:44:00')
@pytest.mark.parametrize("date, tz, result", [
    ('2017-01-27 01:43:00', pytz.utc, '1:43'),
    ('2017-01-27 01:43:00 +5', moscow, '23:43'),
    ('2017-01-27 01:43:00 +3', moscow, '1:43'),
    ('2016-01-26 22:44:00 +0', pytz.utc, '22:44'),
    ('2016-01-27 22:44:00 +0', moscow, '1:44'),
])
def test_human_time_with_tz(date, tz, result):
    assert human_time(parse(date), tz=tz) == result


@pytest.mark.parametrize('dt, lang, grams, result', [
    (now, 'ru', ('gen',), 'января'),
    (now, 'ru', ('nomn',), 'январь'),
    (now, 'ru', ('nomn', 'pl'), 'январи'),
])
def test_human_month(dt, lang, grams, result):
    assert human_month(dt, lang, *grams) == result


@pytest.mark.parametrize('value, result', [
    (29.7, 'меньше минуты'),
    (30, '1 минуту'),
    (65.7, '1 минуту'),
    (119, '2 минуты'),
    (3600, '1 час'),
    (3600.0 * 5 + 60 * 7, '5 часов 7 минут'),
    (1e+20, 'очень много времени'),
])
def test_human_seconds(value, result):
    assert human_seconds(value) == result


@pytest.mark.parametrize('value, result', [
    (0.4, 'меньше 100 метров'),
    (0.9, 'меньше 100 метров'),
    (49, 'меньше 100 метров'),
    (51, '100 метров'),
    (523.0, '500 метров'),
    (127.7, '100 метров'),
    (1119.1, '1 километр 100 метров'),
    (3000.0, '3 километра'),
])
def test_human_meters(value, result):
    assert human_meters(value) == result


@pytest.mark.parametrize('geo, result', [
    ({'city': 'Москва'}, 'Москва'),
    ({'city': 'Москва', 'street': 'Льва Толстого', 'in_user_city': True}, 'Льва Толстого'),
    ({'city': 'Москва', 'street': 'Льва Толстого', 'in_user_city': False}, 'Москва, Льва Толстого'),
    ({'city': 'Москва', 'street': 'Льва Толстого', 'house': 16, 'in_user_city': True}, 'Льва Толстого 16'),
    ({'address_line': 'xxx', 'house': 13}, 'xxx'),
])
def test_geo_city_address(geo, result):
    assert geo_city_address(geo) == result


@pytest.mark.parametrize('geo, result', [
    ({'city': 'Москва'}, 'в городе Москва'),
    ({'city': 'Москва', 'city_cases': {'preposition': 'в', 'prepositional': 'Москве'}}, 'в Москве'),
    ({'city': 'Москва', 'city_cases': {'preposition': 'в', 'prepositional': 'Москве'}}, 'в Москве'),
    ({'city': 'Москва', 'city_cases': {'preposition': 'в', 'nominative': 'Москве'}}, 'в городе Москва'),
    pytest.param({}, '', marks=pytest.mark.xfail(reason='Not a city')),
])
def test_city_prepcase(geo, result):
    assert city_prepcase(geo) == result


@pytest.mark.parametrize('str, grams, result', [
    ('Москва', ('gen',), 'Москвы'),
    ('понедельник', ('gen', 'plur'), 'понедельников'),
])
def test_inflect(str, grams, result):
    assert inflect(str, *grams) == result


@pytest.mark.parametrize('dt, fmt, result', [
    (now, '%Y', '2017'),
    (now, '%H:%M', '19:45'),
    (now, '', ''),
])
def test_format_datetime(dt, fmt, result):
    assert format_datetime(dt, fmt) == result


@pytest.mark.parametrize('dt, lang, result', [
    (now, 'ru', 'среда'),
    (now + timedelta(days=1), 'ru', 'четверг'),
    (now - timedelta(days=1), 'ru', 'вторник'),
])
def test_format_weekday(dt, lang, result):
    assert format_weekday(dt, lang) == result


@pytest.mark.parametrize('input, output', [
    ('lol', 'Lol'),
    ('', ''),
    ('l', 'L'),
    ('L', 'L'),
    ('loL', 'LoL'),
    ('лол', 'Лол'),
])
def test_capitalize_first(input, output):
    assert capitalize_first(input) == output


@pytest.mark.parametrize('input, output', [
    ('123', '123'),
    ('', ''),
    ('12345', '1 2 3 4 5'),
    ('12345 aaa', '1 2 3 4 5 aaa'),
    ('12345, 123456', '1 2 3 4 5, 1 2 3 4 5 6'),
    ('Информация о партнёре. Название: Аробакаш. Регистрационный номер ОГРН: 1177746224886', 'Информация о партнёре. Название: Аробакаш. Регистрационный номер ОГРН: 1 1 7 7 7 4 6 2 2 4 8 8 6'),
    ('Адрес: 119021, Москва, ул. Льва Толстого, 16', 'Адрес: 1 1 9 0 2 1, Москва, ул. Льва Толстого, 16')
])
def test_split_long_numbers(input, output):
    assert split_long_numbers(input) == output


@pytest.mark.parametrize('str, num, case, result', [
    ('маршрут', 2, 'nomn', 'маршрута'),
    ('маршрут', 5, 'nomn', 'маршрутов'),
    ('маршрут', 1, 'nomn', 'маршрут'),
    ('маршрут', 1, 'datv', 'маршруту'),
    ('маршрут', 2, 'datv', 'маршрутам'),
    ('маршрут', 2, 'ins', 'маршрутами'),
    ('маршрут', 5, 'acc', 'маршрутов'),
    ('минута', 1, 'nomn', 'минута'),
    ('минута', 1, 'acc', 'минуту'),
])
def test_pluralize(str, num, case, result):
    assert pluralize(str, num, case) == result


@pytest.mark.parametrize('str, num, result', [
    ('маршрута', 2, 'маршрут'),
    ('маршрутов', 5, 'маршрут'),
    ('маршрут', 1, 'маршрут'),
])
def test_singularize(str, num, result):
    assert singularize(str, num) == result


@pytest.mark.parametrize('fixed_list, optional_list, required_list', [
    (['a', 'b'], ['c', 'd'], ['e', 'f']),
    (['a', 'b'], ['c', 'd'], []),
    (['a', 'b'], [], ['e', 'f']),
    (['a', 'b'], [], []),
    ([], ['c', 'd'], ['e', 'f']),
    ([], ['c', 'd'], []),
    ([], [], ['e', 'f']),
    ([], [], []),
])
def test_shuffle(fixed_list, optional_list, required_list):
    found_elements = set()
    all_elements = set(fixed_list)
    all_elements.update(optional_list)
    all_elements.update(required_list)
    for x in range(1000):
        result = shuffle(fixed_list, optional_list, required_list)
        found_elements.update(result)

        if len(fixed_list) > 0:
            idx = result.index(fixed_list[0])
            assert idx is not None
            for i in range(1, len(fixed_list)):
                assert result[idx + i] == fixed_list[i]

        for z in required_list:
            assert z in result

    assert all_elements == found_elements


@pytest.mark.parametrize('input_list, result', [
    (['a', 'b', 'c'], 'a b c'),
    (['a', 'b', ''], 'a b'),
    (['a', 'b'], 'a b'),
    (['a ', 'b ', 'c'], 'a b c'),
    ([' a', '  b', ''], 'a b'),
    (['a ', ' b '], 'a b'),
    ([], ''),
    ([''], ''),
    (['a'], 'a'),
])
def test_join(input_list, result):
    assert join(input_list) == result


@pytest.mark.parametrize('input, output', [
    (':thumbsup:', '👍'),
    (':middle_finger:', '🖕'),
])
def test_emojize(input, output):
    assert emojize(input) == output


def _error_message(prefix, dt, source, generated):
    return '%s is not equal for %s. "%s" versus generated "%s"' % (prefix, json.dumps(dt), source, generated)


@pytest.mark.parametrize('dt, phrase, voice_phrase, is_relative',
                         load_data_from_file('data/datetime_raw_examples.json'))
def test_render_datetime_raw(dt, phrase, voice_phrase, is_relative):
    random.seed(2)

    render_result = render_datetime_raw(dt)
    assert render_result.text == phrase, _error_message('Text', dt, phrase, render_result.text)
    assert render_result.voice == voice_phrase, _error_message('Voice', dt, voice_phrase, render_result.voice)
    assert relative_datetime_raw(dt) == is_relative


@pytest.mark.parametrize('obj, key, result', [
    ({'a': {'b': 'x'}}, 'a.b', 'x'),
    ({'a': {'b': 'x'}}, 'a.b.c', ''),
    ({'a': {'b': 'x'}}, 'a.c', ''),
    ({'a': {'b': 'x'}, 'd': 'test'}, 'd', 'test'),
])
def test_get_item(obj, key, result):
    assert get_item(obj, key) == result


@pytest.mark.parametrize('obj, key, result', [
    ({'a': {'b': 'x'}}, 'a.b', 'x'),
    ({'a': {'b': 'x'}}, 'a.b.c', '<default>'),
    ({'a': {'b': 'x'}}, 'a.c', '<default>'),
    ({'a': {'b': 'x'}, 'd': 'test'}, 'd', 'test'),
])
def test_get_item_default(obj, key, result):
    assert get_item(obj, key, '<default>') == result


@pytest.mark.parametrize('url, host', [
    ('http://example.com/path?a=1', 'example.com'),
    ('https://example.com', 'example.com'),
    ('wss://example.com:123/a/b/c/', 'example.com'),
    ('http://example.com/index.php', 'example.com'),
    ('http://xn----8sbiecm6bhdx8i.xn--p1ai/index.php', 'сезоны-года.рф'),
    ('http://xn--b1agh1afp.xn--p1ai/index.php', 'привет.рф'),
    ('//example.com/', 'example.com'),
    ('www.bolshoyvopros.ru/questions/1380576-kto-takaja-marija-orlova-skolko-let-biografija-zamuzhem-est-deti.html', '')
])
def test_hostname(url, host):
    assert hostname(url) == host


@pytest.mark.parametrize('obj, result', [
    ({'a': {'b': 'x', 'c': 'y'}}, '{"a":{"c":"y","b":"x"}}'),
    ({'a': ['b', 'c', {'a': 123}]}, '{"a":["b","c",{"a":123}]}'),
    ([{'a': {'b': 'x'}}, {'a': 1}, ['a', {'b': 2.3}]], '[{"a":{"b":"x"}},{"a":1},["a",{"b":2.3}]]'),
    (None, 'null'),
    (True, 'true'),
    (1, '1'),
    (1.23, '1.23'),
    ('1.23', '"1.23"'),
])
def test_to_json(obj, result):
    assert to_json(obj) == result


@pytest.mark.parametrize('obj, result', [
    (""" Hello "XYZ" this 'is' a test & so <on> """,
        ' Hello &quot;XYZ&quot; this &apos;is&apos; a test &amp; so &lt;on&gt; '),
    ('0\\0', '0&#92;0'),
    ('1.\n2.', '1.<br/>2.'),
])
def test_html_escape(obj, result):
    assert html_escape(obj) == result


@pytest.mark.parametrize('units, case, voice, text', [
    ({'hours': 1}, 'acc', '#acc 1 час', '1 час'),
    ({'hours': 2, 'minutes': 2}, 'acc', '#acc 2 часа #acc 2 минуты', '2 часа 2 минуты'),
    ({'hours': 5, 'seconds': 5}, 'acc', '#acc 5 часов #acc 5 секунд', '5 часов 5 секунд'),
    (
        {'hours': 11, 'minutes': 22, 'seconds': 41},
        'acc',
        '#acc 11 часов #acc 22 минуты #acc 41 секунду',
        '11 часов 22 минуты 41 секунду',
    ),
    ({'minutes': 1}, 'acc', '#acc 1 минуту', '1 минуту'),
    ({'minutes': 3, 'seconds': 7}, 'acc', '#acc 3 минуты #acc 7 секунд', '3 минуты 7 секунд'),
    ({'seconds': 1}, 'acc', '#acc 1 секунду', '1 секунду'),
    (
        {'hours': 11, 'minutes': 22, 'seconds': 41},
        'gen',
        '#gen 11 часов #gen 22 минут #gen 41 секунды',
        '11 часов 22 минут 41 секунды',
    ),
])
def test_render_units_time(units, case, voice, text):
    r = render_units_time(units, case=case)
    assert r.text == text
    assert r.voice == voice


@pytest.mark.parametrize('weekdays, text', [
    ({'weekdays': [1]}, 'в понедельник'),
    ({'weekdays': [2, 3]}, 'во вторник и среду'),
    ({'weekdays': [4, 5, 6]}, 'в четверг, пятницу и субботу'),
    ({'weekdays': [1, 2, 3, 4, 5]}, 'в будни'),
    ({'weekdays': [6, 7]}, 'в выходные'),
    ({'weekdays': [7], 'repeat': True}, 'по воскресеньям'),
    ({'weekdays': [5, 7], 'repeat': True}, 'по пятницам и воскресеньям'),
    ({'weekdays': [2, 3, 5], 'repeat': True}, 'по вторникам, средам и пятницам'),
    ({'weekdays': [1, 2, 3, 4, 5], 'repeat': True}, 'по будням'),
    ({'weekdays': [6, 7], 'repeat': True}, 'по выходным'),
    ({'weekdays': [1, 2, 3, 4, 5, 6, 7]}, 'каждый день'),
])
def test_render_weekday_type(weekdays, text):
    assert render_weekday_type(weekdays) == text


def test_tts_domain():
    assert tts_domain('Включи Pink Floyd', 'music') == '<[domain music]>Включи Pink Floyd<[/domain]>'


@pytest.mark.parametrize('text, num', [
    ('слово 123 слово', 3),
    ('слово', 1),
    ('123', 1),
    ('! слово - 123, слово', 3),
    ('слово:', 1),
    ('123- - .', 1),
    ('- - .', 0)
])
def test_number_of_readable_tokens(text, num):
    assert number_of_readable_tokens(text) == num


@pytest.mark.parametrize('value, result', [
    (1, '1 год'),
    (2, '2 года'),
    (5, '5 лет'),
    (12, '12 лет'),
    (19, '19 лет'),
    (23, '23 года'),
    (39, '39 лет'),
    (100, '100 лет'),
    (651, '651 год'),
    ('', ''),
    ('хз', ''),
    (None, ''),
    ([], '')
])
def test_human_age(value, result):
    assert human_age(value) == result


@pytest.mark.parametrize('weekday, text', [
    (1, 'пн'),
    (2, 'вт'),
    (7, 'вс'),
])
def test_render_short_weekday_name(weekday, text):
    assert render_short_weekday_name(weekday) == text


@pytest.mark.parametrize("s, width, result", [
    ('', 0, ''),
    ('', 10, ''),
    ('двадцатичетырёхбуквенное!', 20, 'двадцатичетырёхбуквенное!'),
    ('двадцатичетырёхбуквенное', 20, 'двадцатичетырёхбуквенное'),
    ('привет как дела', 0, '...'),
    ('привет как дела', 1, 'привет...'),
    ('привет как дела', 3, 'привет...'),
    ('привет как дела', 6, 'привет...'),
    ('привет как дела', 7, 'привет как...'),
    ('привет как дела', 13, 'привет как дела'),
    ('привет как дела', 100, 'привет как дела'),
    ('привет. как? дела!', 3, 'привет...'),
    ('привет. как? дела!', 9, 'привет. как...'),
    ('привет. как? дела!', 100, 'привет. как? дела!'),
])
def test_trim_with_ellipsis(s, width, result):
    assert trim_with_ellipsis(s, width) == result
