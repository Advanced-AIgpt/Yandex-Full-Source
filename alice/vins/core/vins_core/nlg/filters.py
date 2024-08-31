# coding: utf-8
from __future__ import unicode_literals

import random
import itertools
import json
import re
import string
import logging
from datetime import datetime
from urlparse import urlparse

import attr
import emoji
import pytz
from dateutil.parser import parse as parse_dt

from vins_core.utils.config import get_setting
from vins_core.utils.lemmer import Inflector
from vins_core.utils.datetime import parse_tz
from vins_core.utils.strings import smart_unicode
from vins_core.nlg.nlg_extension import (ONLY_VOICE_MARK, ONLY_TEXT_MARK)

_inflector = Inflector('ru')

logger = logging.getLogger(__name__)
NOW_TIMESTAMP = get_setting('NOW_TIMESTAMP', default='')


def _datetime_now(*args, **kwargs):
    if NOW_TIMESTAMP:
        return datetime.utcfromtimestamp(float(NOW_TIMESTAMP))
    return datetime.now(*args, **kwargs)


def format_datetime(date, fmt=''):
    return date.strftime(fmt)


def seconds_diff(time1, time2):
    diff = parse_dt(time2) - parse_dt(time1)
    return diff.total_seconds()


def insert_spaces_between_chars(st):
    return ' '.join(st)


def insert_comas_between_chars(st):
    return ', '.join(st)


def split_long_numbers(st):
    tokens = list(filter(lambda x: x != '', re.split(r'[,\.\?\:\ ]', st)))
    trash = list(filter(lambda x: x != '', re.split(r'[^,\.\?\:\ ]', st)))
    trash.append('')
    get_res = lambda st: ' '.join(st) if st.isdigit() and len(st) > 4 else st
    new_tokens = list(map(get_res, tokens))
    new_st = [z[0]+z[1] for z in zip(new_tokens, trash)]
    return ''.join(new_st)


def percent_cases(num):
    base = str(num)+' процент'
    last_dig = num % 10
    if (last_dig >= 2 and last_dig <= 4 and (num <= 10 or num >= 20)):
        base += 'а'
    elif (last_dig == 0 or last_dig >= 5 or num >= 10 and num <= 20):
        base += 'ов'
    return base


def _choose_tense_datetime_raw(dt):
    # datetime_raw case, currently only completely relative datetimes are supported

    date_keys = ['years', 'months', 'weeks', 'days']
    time_keys = ['hours', 'minutes', 'seconds']

    is_relative = False

    if dt.get('date_relative'):
        is_relative = True
        for key in date_keys:
            shift = dt.get(key, 0)
            if shift < 0:
                return -1
            elif shift > 0:
                return 1

    if dt.get('time_relative'):
        is_relative = True
        for key in time_keys:
            shift = dt.get(key, 0)
            if shift < 0:
                return -1
            elif shift > 0:
                return 1

    for key, value in dt.items():
        if key.endswith('_relative') and value:
            is_relative = True
            truncated_key = key[0:-9]
            shift = dt.get(truncated_key, 0)
            if shift < 0:
                return -1
            elif shift > 0:
                return 1

    if not is_relative:
        raise ValueError('Non relative datetime_raw is not supported at the moment: %s' % json.dumps(dt))

    return 0


def choose_tense(dt, consider_time=True, future_as_present=False, tz=pytz.UTC):
    """
    Choose verb tense for the time moment
    :param dt: time moment for which tense must be chosen,
    type dt: datetime or datetime_raw
    :param consider_time: if set to False all moments within one day would be considered as same
    :type consider_time: bool
    :param future_as_present: if set to True 0 will be returned even if future tense was chosen
    :type future_as_present: bool
    :return: -1 - past, 0 - present, 1 - future
    :rtype: int
    """

    if isinstance(dt, dict):
        return _choose_tense_datetime_raw(dt)

    dt = _convert_timezone(dt, tz)

    if not consider_time:
        now = _datetime_now(dt.tzinfo).date()
        return -1 if dt.date() < now else 0 if ((dt.date() == now) or future_as_present) else 1

    now = _datetime_now(dt.tzinfo)
    return -1 if dt < now else 0 if ((dt == now) or future_as_present) else 1


def markup(s, mark):
    stripped = s.strip()
    return '\'%s\'(%s)' % (stripped, mark) if stripped else ''


weekday_mapping = {
    1: 'понедельник',
    2: 'вторник',
    3: 'среда',
    4: 'четверг',
    5: 'пятница',
    6: 'суббота',
    7: 'воскресенье',
}


weekday_mapping_short = {
    1: 'пн',
    2: 'вт',
    3: 'ср',
    4: 'чт',
    5: 'пт',
    6: 'сб',
    7: 'вс',
}


weekday_gender = {
    1: 'm',
    2: 'm',
    3: 'f',
    4: 'm',
    5: 'f',
    6: 'f',
    7: 'n',
}


month_mapping = {
    1: 'январь',
    2: 'февраль',
    3: 'март',
    4: 'апрель',
    5: 'май',
    6: 'июнь',
    7: 'июль',
    8: 'август',
    9: 'сентябрь',
    10: 'октябрь',
    11: 'ноябрь',
    12: 'декабрь',
}

datetime_raw_replacements_prefix = {
    'через #acc 1 год': 'через год',
    'через #acc 1 месяц': 'через месяц',
    'через #acc 1 неделю': 'через неделю',
    'через #acc 1 час': 'через час',
    '#acc 1 год назад': 'год назад',
    '#acc 1 месяц назад': 'месяц назад',
    '#acc 1 неделю назад': 'неделю назад',
    '#acc 1 час назад': 'час назад',
}


@attr.s
class VoiceAndText(object):
    voice = attr.ib()
    text = attr.ib()


def _datetime_raw_special_cases(dt):
    if len(dt.keys()) == 3 and dt.get('weeks') in [-1, 1] and dt.get('weeks_relative') and dt.get('weekday'):
        ord_adj = random.choice(['прошлый', 'прошедший'] if dt['weeks'] == -1 else ['будущий', 'следующий'])

        return 'в %s %s' % (
            inflect(ord_adj, 'acc', weekday_gender[dt['weekday']]),
            inflect(weekday_mapping[dt['weekday']], 'acc')
        )

    if len(dt.keys()) == 2:
        if dt.get('seconds') == 0 and dt.get('seconds_relative', False):
            return 'сейчас'
        elif dt.get('minutes') == 0 and dt.get('minutes_relative', False):
            return 'сейчас'
        elif dt.get('hours') == 0 and dt.get('hours_relative', False):
            return 'сейчас'
        elif dt.get('days_relative', False):
            days = dt.get('days')
            if days == 0:
                return 'сегодня'
            elif days == 1:
                return 'завтра'
            elif days == 2:
                return 'послезавтра'
            elif days == -1:
                return 'вчера'
            elif days == -2:
                return 'позавчера'
        elif dt.get('weeks') == 0 and dt.get('weeks_relative', False):
            return 'на этой неделе'
        elif dt.get('months') == 0 and dt.get('months_relative', False):
            return 'в этом месяце'
        elif dt.get('years') == 0 and dt.get('years_relative', False):
            return 'в этом году'

    return None


time_keys = {'hours', 'minutes', 'seconds'}

all_datetime_raw_components = [
    ('years', 'год'),
    ('months', 'месяц'),
    ('weeks', 'неделя'),
    ('days', 'день'),
    ('hours', 'час'),
    ('minutes', 'минута'),
    ('seconds', 'секунда')
]


def _process_relative_part_of_datetime_raw(dt):
    tense = None

    absolute_date_keys = set()
    absolute_time_keys = set()

    date_relative = dt.get('date_relative', False)
    time_relative = dt.get('time_relative', False)

    relative_part = []
    for key, name in all_datetime_raw_components:
        is_relative = (time_relative if key in time_keys else date_relative) or dt.get('%s_relative' % key, False)
        if is_relative:
            quantity = dt.get(key)
            if quantity:
                current_tense = 1 if quantity > 0 else -1
                if tense and current_tense != tense:
                    raise ValueError('Relative parts of the datetime_raw have different signs: %s' % json.dumps(dt))
                else:
                    tense = current_tense

                abs_quantity = abs(quantity)
                phrase = '#acc %d %s' % (abs_quantity, pluralize(name, abs_quantity, 'acc'))
                phrase = phrase.replace('годов', 'лет')
                relative_part.append(phrase)
            else:
                raise ValueError('Zero relatives are allowed only in few special cases: %s' % json.dumps(dt))
        elif key in time_keys:
            if key in dt:
                absolute_time_keys.add(key)
        elif key in dt:
            absolute_date_keys.add(key)

    return relative_part, absolute_date_keys, absolute_time_keys, tense


def safe_render_datetime_raw(dt):
    try:
        if not dt:
            return None

        return render_datetime_raw(dt)
    except ValueError:
        return None


def render_datetime_raw(dt):
    special_case_result = _datetime_raw_special_cases(dt)
    if special_case_result:
        return VoiceAndText(special_case_result, special_case_result)

    relative_part, absolute_date_keys, absolute_time_keys, tense = _process_relative_part_of_datetime_raw(dt)

    result = []

    if relative_part:
        if len(relative_part) > 1:
            relative_part.insert(-1, 'и')

        if tense > 0:
            result.append('через')

        result += relative_part

        if tense < 0:
            result.append('назад')

    rel_part_length = len(result)

    days = dt.get('days')
    months = dt.get('months')
    if 'days' in absolute_date_keys:
        if 'months' in absolute_date_keys and days and months:
            result.append('#gen %d %s' % (days, inflect(month_mapping[months], 'gen')))
        elif days:
            result.append('на #acc %d-й день' % days)
    elif 'months' in absolute_date_keys and months:
        result.append('в %s' % inflect(month_mapping[months], 'abl'))

    if 'weeks' in absolute_date_keys and dt.get('weeks'):
        raise ValueError('Absolute weeks are currently not supported: %s' % json.dumps(dt))

    if 'years' in absolute_date_keys:
        years = dt.get('years')
        if years:
            if len(result) > rel_part_length:
                result.append('#gen %d года' % years)
            else:
                result.append('в #loc %d году' % years)

    weekday = dt.get('weekday')

    if weekday:
        if weekday == 2:
            result.append('во вторник')
        else:
            result.append('в %s' % inflect(weekday_mapping[weekday], 'acc'))

    if absolute_time_keys:
        if 'hours' in absolute_time_keys:
            simple_datetime = datetime(1970, 01, 01, dt.get('hours', 0), dt.get('minutes', 0), dt.get('seconds', 0))
            result.append(
                'в %s' % human_time(simple_datetime)
            )
        else:
            result.append('в')
            if 'minutes' in absolute_time_keys:
                minutes = dt.get('minutes', 0)
                result.append('#acc %d %s' % (minutes, pluralize('минута', minutes, 'acc')))
            if 'seconds' in absolute_time_keys:
                seconds = dt.get('seconds', 0)
                result.append('#acc %d %s' % (seconds, pluralize('секунда', seconds, 'acc')))

    voice_result = ' '.join(result)

    for k, v in datetime_raw_replacements_prefix.items():
        if voice_result.startswith(k):
            voice_result = v + voice_result[len(k):]
            break

    text_result = re.sub(r'#[a-z]* ', '', voice_result, flags=re.UNICODE)

    return VoiceAndText(voice_result, text_result)


def human_day_rel(dt, tz=pytz.UTC):
    dt = _convert_timezone(dt, tz)
    now = _datetime_now(dt.tzinfo).date()
    delta = (dt.date() - now).days

    if delta == 0:
        return 'сегодня'
    elif delta == 1:
        return 'завтра'
    elif delta == 2:
        return 'послезавтра'
    elif delta == -1:
        return 'вчера'
    elif delta == -2:
        return 'позавчера'
    else:
        return _human_date(dt, with_year=dt.year != now.year)


def human_date(dt, tz=pytz.UTC):
    dt = _convert_timezone(dt, tz)
    now = _datetime_now(dt.tzinfo).date()
    return _human_date(dt, with_year=dt.year != now.year)


def _convert_timezone(dt, tz):
    if isinstance(tz, basestring):
        tz = parse_tz(tz)

    if dt.tzinfo is None:
        dt = tz.localize(dt)
    else:
        dt = dt.astimezone(tz)

    return dt


def _human_date(dt, with_year=False):
    res = [
        dt.day,
        human_month(dt, 'ru', 'gen')
    ]

    if with_year:
        res.append(dt.year)
        res.append('года')

    return ' '.join(map(unicode, res))


def _human_time(hour, minute, case='nomn'):
    mapping = {
        '0:30': '0:30',
        '1:30': '1:30 ночи',
        '2:00': '2 ночи',
        '2:30': '2:30 ночи',
        '3:00': '3 ночи',
        '3:30': '3:30 ночи',
        '4:00': '4 утра',
        '4:30': '4:30 утра',
        '5:00': '5 утра',
        '5:30': '5:30 утра',
        '6:00': '6 утра',
        '6:30': '6:30 утра',
        '7:00': '7 утра',
        '7:30': '7:30 утра',
        '8:00': '8 утра',
        '8:30': '8:30 утра',
        '9:00': '9 утра',
        '9:30': '9:30 утра',
        '10:00': '10 утра',
        '10:30': '10:30 утра',
        '11:00': '11 утра',
        '11:30': '11:30 утра',
        '12:00': '12 часов дня',
        '12:30': '12:30 дня',
        '17:00': '5 вечера',
        '18:00': '6 вечера',
        '19:00': '7 вечера',
        '20:00': '8 вечера',
        '21:00': '9 вечера',
        '22:00': '10 вечера',
        '23:00': '11 вечера',
    }
    str_ = '%d:%02d' % (hour, minute)
    case = case or 'nomn'
    if minute == 0:
        if hour == 0:  # midnight
            if case in ['nomn', 'nom', 'acc', 'accs']:
                return 'полночь'
            else:
                return inflect('полуночь', case)
        else:
            if str_ in mapping:
                return mapping[str_]
            elif hour == 1:  # 01:00
                return '1 %s ночи' % inflect('час', case)
            else:  # 13:00, 14:00, 15:00, 16:00
                hour = hour - 12
                hours_str = pluralize('час', hour, case)
                return '%d %s дня' % (hour, hours_str)
    else:
        return mapping.get(str_, str_)


def human_time_raw(time, case='nomn'):
    return _human_time(time.get('hours', 0), time.get('minutes', 0), case=case)


def human_time_raw_text_and_voice(time, case='nomn'):
    case = case or 'nomn'
    hour = time.get('hours', 0)
    minutes = time.get('minutes', 0)
    voice = _human_time(hour, minutes, case=case)
    if re.match(r'\d+.*', voice):
        voice = '#{} {}'.format(case, _human_time(hour, minutes, case=case))

    text = '%d:%02d' % (hour, minutes)
    return VoiceAndText(voice=voice, text=text)


def human_time(dt, case='nomn', tz=pytz.UTC):
    dt = _convert_timezone(dt, tz)
    return _human_time(dt.hour, dt.minute, case)


def human_seconds(seconds):
    if seconds > 60 * 60 * 24 * 7:
        return 'очень много времени'

    minutes = int(round(seconds / 60.0))
    hours = minutes // 60

    if minutes == 0:
        return 'меньше минуты'

    result = ''

    if hours > 0:
        minutes -= 60 * hours
        hours_str = pluralize('час', hours, 'acc')
        result += '%d %s' % (hours, hours_str)

    if minutes > 0:
        minutes_str = pluralize('минута', minutes, 'acc')
        if len(result) > 0:
            result += ' '
        result += '%d %s' % (minutes, minutes_str)

    return result


def human_seconds_short(seconds):
    if seconds > 60 * 60 * 24 * 7:
        return 'очень много времени'

    minutes = int(round(seconds / 60.0))
    hours = minutes // 60

    if minutes == 0:
        return '1 мин'

    result = ''

    if hours > 0:
        minutes -= 60 * hours
        result += '%d ч' % (hours)

    if minutes > 0:
        if len(result) > 0:
            result += ' '
        result += '%d мин' % (minutes)

    return result


def human_meters(meters, only_km_threshold=5000):
    """ Convert a number of meters to a string like 'X meters' or 'X kilometers Y meters'
    with meters rounded to the nearest hundred (so, 50 meters will be represented as 100).

    :param meters: the number of meters to represent as a string
    :type meters: int or float
    :param only_km_threshold: the number of meters after which the meters are rounded away
    :type only_km_threshold: int or none

    :returns: a string representing the given number of meters.
    :rtype: str
    """
    if only_km_threshold is not None and meters >= max(1000, only_km_threshold):
        meters = int(round(meters, -3))
    else:
        meters = int(round(meters, -2))

    if meters == 0:
        return 'меньше 100 метров'

    kilometers = meters // 1000
    meters -= 1000 * kilometers

    result = ''

    if kilometers > 0:
        kilometers_str = pluralize('километр', kilometers, 'acc')
        result += '%d %s' % (kilometers, kilometers_str)

    if meters > 0:
        meters_str = pluralize('метр', meters, 'acc')
        if len(result) > 0:
            result += ' '
        result += '%d %s' % (meters, meters_str)

    return result


def human_meters_short(meters, only_km_threshold=5000):
    """ Convert a number of meters to a string like 'X m' or 'X km Y m'
    with meters rounded to the nearest hundred (so, 50 meters will be represented as 100).

    :param meters: the number of meters to represent as a string
    :type meters: int or float
    :param only_km_threshold: the number of meters after which the meters are rounded away
    :type only_km_threshold: int or none

    :returns: a string representing the given number of meters.
    :rtype: str
    """
    if only_km_threshold is not None and meters >= max(1000, only_km_threshold):
        meters = int(round(meters, -3))
    else:
        meters = int(round(meters, -2))

    if meters == 0:
        return '&lt; 100 м'  # '< 100 м'

    kilometers = meters // 1000
    meters -= 1000 * kilometers

    result = ''

    if kilometers > 0:
        result += '%d км' % (kilometers)

    if meters > 0:
        if len(result) > 0:
            result += ' '
        result += '%d м' % (meters)

    return result


def render_units_time(units, case='acc'):
    result = ''
    voice_result = ''

    hours = units.get('hours')
    minutes = units.get('minutes')
    seconds = units.get('seconds')

    for value, word in ((hours, 'час'), (minutes, 'минута'), (seconds, 'секунда')):
        if value:
            text = '%d %s' % (value, pluralize(word, value, case))
            if result:
                result += ' '
                voice_result += ' '
            voice_result += ('#%s ' % case) + text
            result += text

    return VoiceAndText(voice=voice_result, text=result)


def render_weekday_type(weekday):
    result = ''
    weekdays = weekday.get('weekdays')

    if not weekdays:
        return result

    should_repeat = weekday.get('repeat', False)

    if weekdays == range(1, 6):
        return 'по будням' if should_repeat else 'в будни'
    if weekdays == range(6, 8):
        return 'по выходным' if should_repeat else 'в выходные'
    if weekdays == range(1, 8):
        return 'каждый день'

    days_text = []

    for day in weekdays:
        case = ('dat', 'plur') if should_repeat else ('acc',)
        days_text.append('%s' % inflect(weekday_mapping[day], *case))

    prefix = 'по' if should_repeat else 'в'
    if weekdays[0] == 2 and not should_repeat:
        prefix = 'во'

    result = prefix + ' ' + days_text[0]

    if len(weekdays) > 1:
        for text in days_text[1:-1]:
            result += ', ' + text
        result += ' и ' + days_text[-1]

    return result


def render_short_weekday_name(weekday):
    return weekday_mapping_short.get(weekday, '')


def geo_city_address(geo, strip_country=True, strip_city=False):
    if not isinstance(geo, dict):
        raise ValueError("Cannot format geo '%s' as an address, should be a dictionary." % geo)

    city = geo.get('city')
    street = geo.get('street')
    house = geo.get('house')
    address_line = geo.get('address_line')
    in_user_city = geo.get('in_user_city', False)  # In case of unknown locality assume the worst
    strip_city = strip_city or in_user_city

    if street:
        if city and not in_user_city:
            result = '%s, %s' % (city, street)
        else:
            result = street
        if house:
            result += ' %s' % house
        return result

    elif address_line:
        if strip_city or strip_country:
            tokens = address_line.split(', ')
            if tokens[0] == geo.get('country') and len(tokens) > 1:
                tokens = tokens[1:]
            if strip_city and tokens[0] == city and len(tokens) > 1:
                tokens = tokens[1:]
            address_line = ', '.join(tokens)
        return address_line

    elif city:
        return city

    raise ValueError("Cannot format geo '%s' as an address, it's missing actual geo info." % geo)


def city_prepcase(geo):
    city = geo.get('city')
    city_cases = geo.get('city_cases')

    if city_cases and 'preposition' in city_cases and 'prepositional' in city_cases:
        return '%s %s' % (city_cases['preposition'], city_cases['prepositional'])
    elif city:
        return 'в городе %s' % city
    else:
        raise ValueError('Geo %s does not contain city info' % geo)


def format_weekday(datetime, lang):
    day = datetime.isoweekday()
    mapping = {
        'ru': {
            1: 'понедельник',
            2: 'вторник',
            3: 'среда',
            4: 'четверг',
            5: 'пятница',
            6: 'суббота',
            7: 'воскресенье',
        },
        'tr': {
            1: 'Pazartesi',
            2: 'Salı',
            3: 'Çarşamba',
            4: 'Perşembe',
            5: 'Cuma',
            6: 'Cumartesi',
            7: 'Pazar'
        }
    }
    return mapping[lang][day]


def human_month(dt, lang, *grams):
    mapping = {
        'ru': {
            1: 'январь',
            2: 'февраль',
            3: 'март',
            4: 'апрель',
            5: 'май',
            6: 'июнь',
            7: 'июль',
            8: 'август',
            9: 'сентябрь',
            10: 'октябрь',
            11: 'ноябрь',
            12: 'декабрь',
        },
        'tr': {
            1: 'Ocak',
            2: 'Şubat',
            3: 'Mart',
            4: 'Nisan',
            5: 'Mayıs',
            6: 'Haziran',
            7: 'Temmuz',
            8: 'Ağustos',
            9: 'Eylül',
            10: 'Ekim',
            11: 'Kasım',
            12: 'Aralık',
        }
    }

    month = mapping[lang][dt.month]
    if grams:
        return inflect(month, *grams)
    else:
        return month


def inflect(words, *grams):
    return _inflector.inflect(words, grams)


def pluralize(words, number, case='nomn'):
    return _inflector.pluralize(words, number, case)


def singularize(words, number):
    return _inflector.singularize(words, number)


def _decapitalize(string):
    return string[0].lower() + string[1:]


def decapitalize_first(input_string):
    string = input_string.lstrip()
    if not string:
        return input_string

    return _decapitalize(string)


def decapitalize_all(input_string):
    string = input_string.strip()
    if not string:
        return input_string
    out = []
    for word in string.split():
        out.append(_decapitalize(word))

    return ' '.join(out)


def _capitalize(string):
    return string[0].upper() + string[1:]


def capitalize_first(input_string):
    string = input_string.lstrip()
    if not string:
        return input_string

    return _capitalize(string)


def capitalize_all(input_string):
    string = input_string.strip()
    if not string:
        return input_string
    out = []
    for word in string.split():
        out.append(_capitalize(word))

    return ' '.join(out)


def shuffle(fixed_input_list, list_of_optional_parts, list_of_required_parts=()):
    res = list(fixed_input_list)
    optionals = [x for x in list_of_optional_parts if random.randint(0, 1) == 1]
    random.shuffle(optionals)
    left = []
    for x in itertools.chain(optionals, list_of_required_parts):
        if random.randint(0, 1) == 1:
            res.append(x)
        else:
            left.append(x)

    return left + res


def join(list_of_parts, delimiter=' '):
    return (delimiter.join(x.strip() for x in list_of_parts)).strip()


def emojize(str, use_aliases=True):
    return emoji.emojize(str, use_aliases=use_aliases)


def get_item(obj, key, default=''):
    if not obj:
        return default

    k, _, subkey = key.partition('.')
    if hasattr(obj, k):
        value = getattr(obj, k)
    elif isinstance(obj, (list, tuple)) and k.isdigit():
        value = obj[int(k)]
    elif k in obj:
        value = obj[k]
    else:
        return default

    if subkey:
        return get_item(value, subkey, default)
    else:
        return value


def hostname(url):
    result_host = urlparse(url).hostname or ''
    if result_host.startswith('xn--'):
        return result_host.decode('idna')
    else:
        return result_host


def to_json(value):
    return json.dumps(value, separators=(',', ':'))


def json_escape(text):
    return json.dumps(text)[1: -1]


html_escape_table = {
    '&': '&amp;',
    '"': '&quot;',
    "'": '&apos;',
    '>': '&gt;',
    '<': '&lt;',
    '\\': '&#92;',
    '\n': '<br/>',
}


def html_escape(text):
    return ''.join(html_escape_table.get(c, c) for c in smart_unicode(text))


def tts_domain(text, domain):
    return '<[domain %s]>%s<[/domain]>' % (domain, text)


def number_of_readable_tokens(text):
    splitted_text = text.split()
    counter = 0
    for s in splitted_text:
        if any((c.isalpha() or c.isdigit() for c in s)):
            counter += 1
    return counter


def human_age(age):
    if not isinstance(age, int):
        if (isinstance(age, str) or isinstance(age, unicode)) and age.isdigit():
            age = int(age)
        else:
            return ''
    rem10 = age % 10
    rem100 = age % 100
    if rem10 == 0 or rem10 >= 5 or (10 < rem100 and rem100 < 20):
        return '%d лет' % age
    if rem10 == 1:
        return '%d год' % age
    return '%d года' % age


def opinions_count(count):
    if not isinstance(count, int):
        if (isinstance(count, str) or isinstance(count, unicode)) and count.isdigit():
            count = int(count)
        else:
            return ''
    rem10 = count % 10
    rem100 = count % 100
    if (rem100 >= 10 and rem100 <= 19) \
       or (rem10 >= 5 and rem10 <= 9) \
       or rem10 == 0:
        return '%d отзывов' % count

    if rem10 == 1:
        return '%d отзыв' % count
    if rem10 >= 2 and rem10 <= 4:
        return '%d отзыва' % count
    return ''


def cut_str_parts(input_str, parts):
    pos = 0
    res = ""
    for p in parts:
        res += input_str[pos:p.start()]
        pos = p.end()
    res += input_str[pos:]
    return res


def cut_voice_tags(arg):
    matches = re.finditer("<%s>(?:.*?)</%s>" % ((ONLY_VOICE_MARK,)*2), arg)
    return cut_str_parts(arg, matches)


def cut_text_tags(arg):
    matches = re.finditer("<%s>(?:.*?)</%s>" % ((ONLY_TEXT_MARK,)*2), arg)
    return cut_str_parts(arg, matches)


def cut_voice_tags_without_text(arg):
    matches = re.finditer("<%s>|</%s>" % ((ONLY_VOICE_MARK,)*2), arg)
    return cut_str_parts(arg, matches)


def cut_text_tags_without_text(arg):
    matches = re.finditer("<%s>|</%s>" % ((ONLY_TEXT_MARK,)*2), arg)
    return cut_str_parts(arg, matches)


# Cuts text tags, then encloses it into voice tag unless it is already there
def only_voice(arg, cleanup_voice_tags=False):
    text = cut_text_tags(arg)
    if cleanup_voice_tags:
        text = cut_voice_tags_without_text(text)
    matches = list(re.finditer("</?(?:"+ONLY_TEXT_MARK+"|"+ONLY_VOICE_MARK+")>", text))
    txlen = len(text)
    if len(matches) > 0:
        for m in matches:
            if m.start() > 0 and m.end() < txlen:
                logger.error('Nlg filter "voice": internal tags found in {}'.format(text))
        return text
    return "<%s>%s</%s>" % (ONLY_VOICE_MARK, text, ONLY_VOICE_MARK)


# Cuts voice tags, then encloses it into text tag unless it is already there
def only_text(arg, cleanup_text_tags=False):
    text = cut_voice_tags(arg)
    if cleanup_text_tags:
        text = cut_text_tags_without_text(text)
    matches = list(re.finditer("</?(?:"+ONLY_TEXT_MARK+"|"+ONLY_VOICE_MARK+")>", text))
    txlen = len(text)
    if len(matches) > 0:
        for m in matches:
            if m.start() > 0 and m.end() < txlen:
                logger.error('Nlg filter "text": internal tags found in {}'.format(text))
        return text
    return "<%s>%s</%s>" % (ONLY_TEXT_MARK, text, ONLY_TEXT_MARK)


def end_with_dot(arg):
    """Adds a dot at the end of sentence in case it has none. Also trims
    spaces at the end.

    """
    s = arg.rstrip()
    if s[-1:] in '.!?':
        return s
    return s + '.'


def end_without_dot(arg):
    """Removes a dot at the end of sentence in case it is there. Also trims
    spaces at the end.

    """
    s = arg.rstrip()
    if s[-1:] != '.':
        return s
    return s[:-1]


def end_without_terminal(arg):
    """Removes a terminal (?!.) at the end of sentence in case it is there.
    Also trims spaces at the end.

    """
    s = arg.rstrip()
    if s[-1:] not in '.!?':
        return s
    return s[:-1]


def end_without_dot_smart(arg):
    """Removes a dot at the end of sentence in case it is there, but only
    if input text is just one sentence. Also trims spaces at the end.

    """
    s = arg.rstrip()
    if s[-1:] != '.' or '. ' in s or '? ' in s or '! ' in s:
        return s
    return s[:-1]


def remove_angle_brackets(arg):
    """Removes all occurences of angled brackets «» in a string. Just the
    brackets, not the content.

    """
    return arg.replace("«", "").replace("»", "")


def trim_with_ellipsis(s, width_limit=20):
    if not s:
        return s
    parts = s.split()
    res = ''
    part_index = 0
    while len(res) < width_limit and part_index < len(parts):
        if len(res) > 0:
            res += ' '
        res += parts[part_index]
        part_index += 1

    if part_index < len(parts):
        res = res.rstrip(string.punctuation)
        res = res + '...'

    return res
