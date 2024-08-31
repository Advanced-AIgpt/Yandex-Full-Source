import itertools
import pytz
import re
from datetime import datetime, timedelta

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
from alice.tests.library.vins_response import DivWrapper
from cached_property import cached_property


def assert_is_not_weather_case(response):
    assert response.scenario != scenario.Weather


def assert_is_details_case(response):
    assert response.scenario == scenario.Weather
    assert response.intent == intent.GetWeatherDetails


def assert_is_common_weather_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherEllipsis]
    if not ellipsis:
        matching_intents += [intent.GetWeather, intent.GetWeatherFast]
    assert response.scenario == scenario.Weather
    assert response.intent in matching_intents


def assert_is_not_common_weather_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherEllipsis]
    if not ellipsis:
        matching_intents += [intent.GetWeather, intent.GetWeatherFast]
    assert response.scenario != scenario.Weather or response.intent not in matching_intents


def assert_is_nowcast_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherNowcastEllipsis]
    if not ellipsis:
        matching_intents += [intent.GetWeatherNowcast, intent.GetWeatherNowcastFast]
    assert response.scenario == scenario.Weather
    assert response.intent in matching_intents


def assert_is_not_nowcast_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherNowcastEllipsis]
    if not ellipsis:
        matching_intents += [intent.GetWeatherNowcast, intent.GetWeatherNowcastFast]
    assert response.scenario != scenario.Weather or response.intent not in matching_intents


def assert_is_prec_map_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherNowcastPrecMapEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherNowcastPrecMap)
    assert response.scenario == scenario.Weather
    assert response.intent in matching_intents


def assert_is_not_prec_map_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherNowcastPrecMapEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherNowcastPrecMap)
    assert response.scenario != scenario.Weather or response.intent not in matching_intents


def assert_is_pressure_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherPressureEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherPressure)
    assert response.scenario == scenario.Weather
    assert response.intent in matching_intents


def assert_is_not_pressure_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherPressureEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherPressure)
    assert response.scenario != scenario.Weather or response.intent not in matching_intents


def assert_is_wind_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherWindEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherWind)
    assert response.scenario == scenario.Weather
    assert response.intent in matching_intents


def assert_is_not_wind_case(response, ellipsis=False):
    matching_intents = [intent.GetWeatherWindEllipsis]
    if not ellipsis:
        matching_intents.append(intent.GetWeatherWind)
    assert response.scenario != scenario.Weather or response.intent not in matching_intents


common_day_parts = ['', 'утром', 'днем', 'вечером', 'ночью', ]
common_when = ['', 'сейчас', 'вчера', 'сегодня', 'завтра', 'послезавтра', 'на выходных', 'на неделю', 'на сегодня и завтра',
               '3 сентября', 'с 3 по 5 сентября', ]
common_where = ['', 'в москве', 'в красноярском крае', 'в штате гавайи', 'на фарерских островах',
                'в городе гусь-хрустальный', 'в поселке березовка', ]

common_contexts = [re.sub(' +', ' ', f'{where} {when} {day_part}').strip()
                          for (where, when, day_part) in itertools.product(common_where, common_when, common_day_parts)]

now_phrases = ['Сейчас', 'В настоящее время', 'В настоящий момент', 'В данный момент', 'В данную минуту', ]


_months = ['', 'января', 'февраля', 'марта', 'апреля', 'мая', 'июня', 'июля', 'августа', 'сентября', 'октября', 'ноября', 'декабря', ]
_weekdays = ['в понедельник', 'во вторник', 'в среду', 'в четверг', 'в пятницу', 'в субботу', 'в воскресенье', ]
_days = [
    'нулевого', 'первого', 'второго', 'третьего', 'четв(е|ё)ртого', 'пятого', 'шестого', 'седьмого', 'восьмого', 'девятого',
    'десятого', 'одиннадцатого', 'двенадцатого', 'тринадцатого', 'четырнадцатого', 'пятнадцатого', 'шестнадцатого', 'семнадцатого', 'восемнадцатого', 'девятнадцатого',
    'двадцатого', 'двадцать первого', 'двадцать второго', 'двадцать третьего', 'двадцать четв(е|ё)ртого', 'двадцать пятого',
    'двадцать шестого', 'двадцать седьмого', 'двадцать восьмого', 'двадцать девятого', 'тридцатого', 'тридцать первого',
]


def get_day(delta=0):
    dt = datetime.today() + timedelta(days=delta)
    return f'{dt.day} {_months[dt.month]}'


def get_month(delta=0):
    dt = datetime.today() + timedelta(days=delta*30)
    return _months[dt.month]


def get_day_regex(command):
    for day in range(31, 0, -1):
        if str(day) in command:
            return command.replace(str(day), _days[day])
    return command


def get_weekday(delta=0):
    dt = datetime.now(pytz.timezone('Europe/Moscow')) + timedelta(days=delta)
    return _weekdays[dt.weekday()]


class WeatherDivCard(DivWrapper):
    @property
    def title(self):
        return self.data[1].title

    @property
    def subtitle(self):
        return self.data[3].text

    @cached_property
    def day_time(self):
        return [_.text for _ in self.data[6][0]]

    @cached_property
    def days(self):
        return [_.first.text for _ in self.data[3]]


class PogodaDialogBase(object):

    owners = ('sparkle', 'abc:weatherbackendvteam',)

    @staticmethod
    def _assert_unknown_where(text, where):
        assert any(text.startswith(phrase) for phrase in [
            'К сожалению,',
            'Извините,',
            'Простите,',
            'Увы, но',
        ])
        assert any(phrase + ' где это ' in text for phrase in [
            'я не могу понять,',
            'я не знаю,',
        ])
        assert f'"{where}"' in text

    @staticmethod
    def _list_eq_re(text_list, re_list, ignore_case=True):
        if len(text_list) != len(re_list):
            return False
        for idx in range(len(text_list)):
            if ignore_case:
                sought = re.search(re_list[idx].replace(',', ''), text_list[idx], re.IGNORECASE)
            else:
                sought = re.search(re_list[idx], text_list[idx])
            if not sought:
                return False
        return True

    @staticmethod
    def _build_searchapp_suggests_re(suggests, command=None, search_suggest_is_last=True):
        result = [r'^👍$', r'^👎$']
        if not command:
            result.extend(suggests)
        else:
            res = []
            for com in [command, get_day_regex(command)]:
                com = com.replace(',', '')
                search_phrase = com[:20] + com[20:].split(' ', 1)[0]
                res.append(r'^🔍 "{}{}\??"$'.format(search_phrase, '.*' if len(com) > len(search_phrase) else ''))
            search_suggest = f'({res[0]}|{res[1]})'
            if search_suggest_is_last:
                # Лупа с поиском последний саджест
                result.extend(suggests)
                result.append(search_suggest)
            else:
                # Лупа с поиском предпоследний саджест
                result.extend(suggests[:-1])
                result.append(search_suggest)
                result.append(suggests[-1])
        return result
