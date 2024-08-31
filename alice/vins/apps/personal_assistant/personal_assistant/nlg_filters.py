# coding: utf-8
from __future__ import unicode_literals

import re
import random
import logging
from datetime import datetime
from StringIO import StringIO

from PIL import Image

from vins_core.ext.base import BaseHTTPAPI
from vins_core.nlg.filters import (
    VoiceAndText,
    inflect,
    pluralize,
    weekday_mapping,
    weekday_gender,
    human_month,
)
from vins_core.utils.datetime import utcnow
from vins_core.utils.lemmer import Inflector
from vins_core.ner.fst_normalizer import normalizer_factory, DEFAULT_RU_NORMALIZER_NAME

_inflector = Inflector('ru')
_http = BaseHTTPAPI(max_retries=2)


logger = logging.getLogger(__name__)

numerals_1_31 = {
    1: 'первый',
    2: 'второй',
    3: 'третий',
    4: 'четвертый',
    5: 'пятый',
    6: 'шестой',
    7: 'седьмой',
    8: 'восьмой',
    9: 'девятый',
    10: 'десятый',
    11: 'одиннадцатый',
    12: 'двенадцатый',
    13: 'тринадцатый',
    14: 'четырнадцатый',
    15: 'пятнадцатый',
    16: 'шестнадцатый',
    17: 'семнадцатый',
    18: 'восемнадцатый',
    19: 'девятнадцатый',
    20: 'двадцатый',
    21: 'двадцать первый',
    22: 'двадцать второй',
    23: 'двадцать третий',
    24: 'двадцать четвертый',
    25: 'двадцать пятый',
    26: 'двадцать шестой',
    27: 'двадцать седьмой',
    28: 'двадцать восьмой',
    29: 'двадцать девятый',
    30: 'тридцатый',
    31: 'тридцать первый',
}


numerals_multiple_ten = {
    10: 'десятый',
    20: 'двадцатый',
    30: 'тридцатый',
    40: 'сороковой',
    50: 'пятидесятый',
    60: 'шестидесятый',
    70: 'семидесятый',
    80: 'восьмидесятый',
    90: 'девяностый',
}


def pluralize_tag(string):
    if string is None:
        return None

    tags = {
        '#kilogram': 'килограмм',
        '#meter': 'метр',
        '#centimeter': 'сантиметр',
        '#gram': 'грамм',
        '#kilometer': 'километр',
        '#minute': 'минута',
        '#square_kilometer': 'квадратный километр',
    }

    regexp = r'(?P<num>\d[\d\s]*?)\s(?P<tag>{tags})'.format(tags='|'.join(tags))
    match = re.search(regexp, string, flags=re.UNICODE)
    if match is None:
        return string

    number = int(''.join(match.group('num').split()))
    tag_word = _inflector.pluralize(tags[match.group('tag')], number)

    # replace tag
    start, end = match.span('tag')
    return string[:start] + tag_word + string[end:]


CASES_FOR_SK = {
    'nomn': 'nom',
    'nom': 'nom',
    'gent': 'gen',
    'gen': 'gen',
    'datv': 'dat',
    'dat': 'dat',
    'accs': 'acc',
    'acc': 'acc',
    'ablt': 'instr',
    'instr': 'instr',
    'ins': 'instr',
    'loct': 'loc',
    'abl': 'loc',
    'loc': 'loc',
}


def inflect_amount_of_money(amount, currency, sub_currency, case, sub_currency_digit_capacity=2,
                            mark_sum_as=None, mark_currency_as=None, speech_hints=False):
    """
    Return "nice" string that represents amount of money in the right case. Last two parameters (mark_sum_as and
    mark_currency_as) are necessary when using this function for automatic nlu generation. When these parameters are set
    amount and currency mentions in the result string will be marked up.

    :param amount: money amount
    :type amount: float or int
    :param currency: main currency name
    :type currency: unicode
    :param sub_currency: second currency name (cents, eurocents etc)
    :type sub_currency: unicode
    :param case: target case of the whole phrase
    :type case: unicode
    :param sub_currency_digit_capacity: digit capacity of the sub_currency, if sub_currency is None will be used
        only to choose how many digits to leave after dot
    :type sub_currency_digit_capacity: int
    :rtype: unicode
    :param mark_sum_as: if set to V sum in the result string will look as 'sum'(V)
        - works only if sub_currency is None or amount is round
    :type mark_sum_as: unicode
    :param mark_currency_as: if set to C currency in the result string will look as 'sum'(C)
        - works only if sub_currency is None or amount is round
    :type mark_currency_as: unicode
    :param speech_hints: if set to True special case hints for tts will be inserted before each numeral
        e.g '#acc 125 рублей #acc 10 копеек'
    :type speech_hints: bool
    """

    rounded_sum = round(amount, sub_currency_digit_capacity)
    rounded_sum_str = ('%f' % rounded_sum).rstrip('0').rstrip('.')

    parts = rounded_sum_str.split('.')
    if speech_hints:
        sk_case = CASES_FOR_SK[case]

    if not sub_currency or len(parts) == 1:
        if speech_hints:
            rounded_sum_str = '#%s %s' % (sk_case, rounded_sum_str)

        if mark_sum_as:
            rounded_sum_str = '\'%s\'(%s)' % (rounded_sum_str, mark_sum_as)

        currency_str = _inflector.pluralize(currency, rounded_sum, case)
        if mark_currency_as:
            currency_str = '\'%s\'(%s)' % (currency_str, mark_currency_as)
        return '%s %s' % (rounded_sum_str, currency_str)
    else:
        # string representation of float can't contain more than one dot
        assert len(parts) == 2

        rounded_sum_str = parts[0]

        sub_sum = int(float('0.' + parts[1]) * pow(10, sub_currency_digit_capacity))
        sub_sum_str = '#%s %d' % (sk_case, sub_sum) if speech_hints else '%d' % sub_sum

        if rounded_sum_str == '0':
            return '%s %s' % (sub_sum_str, _inflector.pluralize(sub_currency, sub_sum, case))

        if speech_hints:
            rounded_sum_str = '#%s %s' % (sk_case, rounded_sum_str)
        return '%s %s %s %s' % (rounded_sum_str, _inflector.pluralize(currency, int(parts[0]), case),
                                sub_sum_str, _inflector.pluralize(sub_currency, sub_sum, case))


def choose_equal_verb(tense, subj_gender):
    equal_verb_data = {
        (-1, 'm'): [('был равен', 'dat'), ('–', 'nom'), ('составлял', 'acc')],
        (-1, 'f'): [('была равна', 'dat'), ('–', 'nom'), ('составляла', 'acc')],
        (0, 'm'): [('равен', 'dat'), ('–', 'nom'), ('составляет', 'acc')],
        (0, 'f'): [('равна', 'dat'), ('–', 'nom'), ('составляет', 'acc')],
    }
    data = random.choice(equal_verb_data[(tense, subj_gender)])
    return {'wordform': data[0], 'gov_case': data[1]}


def try_round_float(str):
    try:
        v = float(str)  # Make sure the value is a valid float
        digits_after_dot = len(str) - str.find('.') - 1
        if digits_after_dot == len(str) or digits_after_dot <= 4:
            return str

        return 'примерно %.4f' % v
    except ValueError:
        # Not a valid float
        return str


def split_cost_with_spaces(cost):
    parts = cost.rsplit('.', 1)
    int_part = '{:,}'.format(int(parts[0])).replace(',', ' ')
    float_part = parts[1] if len(parts) == 2 else None
    return int_part + '.' + float_part if float_part else int_part


def normalize_time_units(time_units):
    seconds_total = time_units.get('hours', 0) * 3600 + time_units.get('minutes', 0) * 60 + time_units.get('seconds', 0)

    seconds = seconds_total % 60
    minutes = (seconds_total / 60) % 60
    hours = seconds_total / 3600

    result = {}
    for unit, unit_label in [(seconds, 'seconds'), (minutes, 'minutes'), (hours, 'hours')]:
        if unit:
            result[unit_label] = unit
    return result


def ceil_seconds(time_units, aggressive=True):
    if time_units.get('seconds', 0) == 0:
        return time_units
    if time_units.get('hours', 0) == 0 and not aggressive:
        return time_units

    result = time_units.copy()
    del result['seconds']
    result['minutes'] = result.get('minutes', 0) + 1
    return normalize_time_units(result)


def alarm_time_format(time, case='nomn'):
    hours = time.get('hours', 0)
    minutes = time.get('minutes', 0)
    period = time.get('period')
    if period == 'pm' and hours != 12:
        hours += 12
    elif period == 'am' and hours == 12:
        hours = 0

    voice = ''
    text = ''
    sk_case = CASES_FOR_SK[case]
    if minutes == 0:
        if hours == 0:
            text = '12 %s ночи' % pluralize('час', 12, case)
            voice = '#%s %s' % (sk_case, text)
        elif hours == 1:
            text = '%s ночи' % inflect('час', case)
            voice = text
        elif hours >= 2:
            text = '%d %s' % (hours, pluralize('час', hours, case))
            if hours in (2, 3):
                text += ' ночи'
            elif 4 <= hours < 12:
                text += ' утра'
            elif hours == 12:
                text += ' дня'

            voice = '#%s %s' % (sk_case, text)
    else:
        voice = '#%s %d %s #%s %d %s' % (
            sk_case, hours, pluralize('час', hours, case),
            sk_case, minutes, pluralize('минута', minutes, case)
        )
        text = '%02d:%02d' % (hours, minutes)
    return VoiceAndText(voice=voice, text=text)


def music_title_shorten(title):
    short_title = re.sub(r'\([^\)]+\)', '', title, flags=re.UNICODE).strip()
    if not short_title:
        return title.strip()

    super_short_title = re.sub(r'^([^,;]+)(,|;).*$', '\\1', short_title, flags=re.UNICODE).strip()
    if not super_short_title:
        return short_title

    return super_short_title


def strip(string):
    return string.strip()


def number_to_word(number):
    if not isinstance(number, int):
        raise ValueError("Argument not a number")
    return normalizer_factory.get_normalizer(DEFAULT_RU_NORMALIZER_NAME).normalize(str(number))


def year_number_to_numeral(year, case='gen'):
    balance = year % 100
    if balance % 10 == 0:
        return number_to_word(year - balance) + ' ' + inflect(numerals_multiple_ten[balance], case, 'm')
    elif 10 <= balance < 20:
        return number_to_word(year - balance) + ' ' + inflect(numerals_1_31[balance], case, 'm')
    else:
        balance = balance % 10
        return number_to_word(year - balance) + ' ' + inflect(numerals_1_31[balance], case, 'm')


def render_date_with_on_preposition(dt):
    """
    Render date with preposition на
    :param dt - date for render
    :type dict, supports absolute keys, relative type for days, weekdays
    """

    if dt.get('days_relative'):
        if dt.get('days') == 0:
            return VoiceAndText('на сегодня', 'на сегодня')
        elif dt.get('days') == 1:
            return VoiceAndText('на завтра', 'на завтра')
        elif dt.get('days') == 2:
            return VoiceAndText('на послезавтра', 'на послезавтра')
        elif dt.get('days') == -1:
            return VoiceAndText('на вчера', 'на вчера')
        elif dt.get('days') == -2:
            return VoiceAndText('на позавчера', 'на позавчера')

    if len(dt.keys()) == 3 and dt.get('weeks') in [-1, 1] and dt.get('weeks_relative') and dt.get('weekday'):
        ord_adj = random.choice(['прошлый'] if dt['weeks'] == -1 else ['следующий'])

        res = 'на %s %s' % (
            inflect(ord_adj, 'acc', weekday_gender[dt['weekday']]),
            inflect(weekday_mapping[dt['weekday']], 'acc')
        )
        return VoiceAndText(res, res)

    weekday = dt.get('weekday')
    if weekday:
        result = 'на %s' % inflect(weekday_mapping[weekday], 'acc')
        return VoiceAndText(result, result)

    day_exists = 'days' in dt
    month_exists = 'months' in dt
    year_exists = 'years' in dt

    now = utcnow().date()
    dt = datetime(year=dt['years'] if 'years' in dt else now.year,
                  month=dt.get('months', 1), day=dt.get('days', 1)).date()
    txt_list, vc_list = ['на'], ['на']

    if day_exists:
        txt_list.extend([dt.day, human_month(dt, 'ru', 'gen')])
        vc_list.extend([inflect(numerals_1_31[dt.day], 'nom', 'n'), human_month(dt, 'ru', 'gen', 'n')])
    elif month_exists:
        txt_list.append(human_month(dt, 'ru', 'acc'))
        vc_list.append(human_month(dt, 'ru', 'acc', 'n'))

    if dt.year != now.year or year_exists and not month_exists:
        txt_list.extend([dt.year, 'года' if month_exists else 'год'])
        vc_list.extend(
            [year_number_to_numeral(dt.year, 'nom' if not month_exists else 'gen'), 'года' if month_exists else 'год']
        )

    return VoiceAndText(' '.join(map(unicode, vc_list)), ' '.join(map(unicode, txt_list)))


def image_ratio(image_url):
    resp = _http.get(image_url)
    width, height = Image.open(StringIO(resp.content)).size
    return float(width) / height
