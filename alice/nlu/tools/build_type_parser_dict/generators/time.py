# coding: utf-8

import json
import re
from base import GeneratorBase
from itertools import product


_HOUR_SYNONYMS = ['часов', 'часиков', 'часа', 'час']

_MINUTE_SYNONYMS = [
    'минут', 'минуток', 'минуточек', 'минуты', 'минутки', 'минуточки',
    'минута', 'минутка', 'минуточка', 'минуту', 'минутку', 'минуточку'
]
_TIME_PERIODS = [
    ('', 'none'),
    ('утра', 'morning'),
    ('утра и', 'morning'),
    ('дня', 'day'),
    ('дня и', 'day'),
    ('вечера', 'evening'),
    ('вечера и', 'evening'),
    ('ночи', 'night'),
    ('ночи и', 'night'),
]


def _strip_repeating_spaces(key):
    return re.sub(r'\s+', ' ', key).strip()


class GeneratorTime(GeneratorBase):
    def __init__(self):
        super(GeneratorTime, self).__init__(name='time')

    def _add_daytime(self, old_dct):
        new_dct = {}
        for key, value in old_dct.iteritems():
            for day_time in ['дня', 'днем', 'пополудни']:
                new_dct[' '.join([key, day_time])] = (value[0], value[1], 'day')
                new_dct[' '.join([day_time, key])] = (value[0], value[1], 'day')

            for day_time in ['ночи', 'ночью', 'пополуночи']:
                new_dct[' '.join([key, day_time])] = (value[0], value[1], 'night')
                new_dct[' '.join([day_time, key])] = (value[0], value[1], 'night')

            for day_time in ['вечера', 'вечером']:
                new_dct[' '.join([key, day_time])] = (value[0], value[1], 'evening')
                new_dct[' '.join([day_time, key])] = (value[0], value[1], 'evening')
            new_dct[' '.join(['вечером', key])] = (value[0], value[1], 'evening')
            new_dct[' '.join(['вечером в', key])] = (value[0], value[1], 'evening')
            new_dct[' '.join(['вечером на', key])] = (value[0], value[1], 'evening')
            new_dct[' '.join([key, 'вечером'])] = (value[0], value[1], 'evening')
            new_dct[' '.join([key, 'на вечер'])] = (value[0], value[1], 'evening')

            for day_time in ['утра', 'утром', 'до полудня']:
                new_dct[' '.join([key, day_time])] = (value[0], value[1], 'morning')
                new_dct[' '.join([day_time, key])] = (value[0], value[1], 'morning')
            new_dct[' '.join(['утром', key])] = (value[0], value[1], 'morning')
            new_dct[' '.join(['утром на ', key])] = (value[0], value[1], 'morning')
            new_dct[' '.join(['утром в', key])] = (value[0], value[1], 'morning')
            new_dct[' '.join([key, 'на утро'])] = (value[0], value[1], 'morning')

            new_dct[' '.join([key, 'сегодня'])] = (value[0], value[1], 'none')
            new_dct[' '.join([key, 'на сегодня'])] = (value[0], value[1], 'none')
            new_dct[' '.join([key, 'на сегодняшний день'])] = (value[0], value[1], 'none')
            new_dct[' '.join(['сегодня на ', key])] = (value[0], value[1], 'none')
            new_dct[' '.join(['сегодня', key])] = (value[0], value[1], 'none')
            new_dct[' '.join(['сегодня на ', key])] = (value[0], value[1], 'none')
            new_dct[' '.join(['сегодня в ', key])] = (value[0], value[1], 'none')
            new_dct[' '.join(['сегодняшний день ', key])] = (value[0], value[1], 'none')
        return new_dct

    def _convert_absolute_time(self, dct):
        new_dct = {}
        for key, value in dct.iteritems():
            h, m, hint = value
            if hint != 'none':
                if hint == 'day' or hint == 'evening':
                    period = 'pm'
                if hint == 'night' or hint == 'morning':
                    period = 'am'
                new_dct[key] = json.dumps({'hours': h, 'minutes': m, 'period': period})
            else:
                new_dct[key] = json.dumps({'hours': h, 'minutes': m})
        return new_dct

    def _generate_absolute_time(self):
        dct = {}
        for h in xrange(24):
            # {h}
            dct[str(h)] = (h, 0, 'none')
            # {h} 00
            dct[' '.join([str(h), '00'])] = (h, 0, 'none')
            # {h} часов
            for hour_names in _HOUR_SYNONYMS:
                dct[' '.join([str(h), hour_names])] = (h, 0, 'none')

            for m in xrange(60):
                value = ((h + 23) % 24, 60 - m, 'none')
                # без {m} минут {h}
                for minute_name in _MINUTE_SYNONYMS:
                    dct[' '.join(['без', str(m), minute_name, str(h)])] = value
                # без {m} {h}
                dct[' '.join(['без', str(m), str(h)])] = value

                value = ((h + 23) % 24, m, 'none')
                for minute_name in _MINUTE_SYNONYMS:
                    # {m} минут {h}
                    dct[' '.join([str(m), minute_name, str(h)])] = value

                value = (h, m, 'none')
                for hour_name in _HOUR_SYNONYMS:
                    for minute_name in _MINUTE_SYNONYMS:
                        # {h} часов {m} минут
                        dct[' '.join([str(h), hour_name, str(m), minute_name])] = value
                # {h} {m}
                dct[' '.join([str(h), str(m)])] = (h, m, 'none')
                if m < 10:
                    dct[' '.join([str(h), '0', str(m)])] = (h, m, 'none')

        for h, name in enumerate(['полпервого', 'полвторого', 'полтретьего', 'полчетвертого', 'полшестого', 'полседьмого',
                                  'полвосьмого', 'полдевятого', 'полдесятого', 'пол-одинадцатого', 'полдвенадцатого',
                                  'полтринадцатого', 'полчетырнадатого', 'полшестнадцатого', 'полсемнадцатого',
                                  'полвосемнадцатого', 'полдевятнадцатого', 'полдвадцатого', 'полдвадцатьпервого',
                                  'полдвадцатьвторого', 'полдвадцатьтретьего', 'полдвадцатьчетвертого']):
            dct[name] = (h, 30, 'none')

        for h in xrange(24):
            for half in ['половиной', 'половинкой', 'половой']:
                dct[' '.join([str(h), 'с', half])] = (h, 30, 'none')
            for half in ['пол', 'половина', 'половинка', 'половине']:
                dct[' '.join([half, str(h + 1)])] = (h, 30, 'none')

        dct['час'] = (1, 0, 'none')

        dct.update(self._add_daytime(dct))

        for h, m in product(xrange(24), xrange(60)):
            for hour_name, minute_name in product([''] + _HOUR_SYNONYMS, [''] + _MINUTE_SYNONYMS):
                for time_period_name, time_period in _TIME_PERIODS:
                    key = ' '.join([str(h), hour_name, time_period_name, str(m), minute_name])
                    key = _strip_repeating_spaces(key)
                    if key not in dct:
                        dct[key] = (h, m, time_period)

        return self._convert_absolute_time(dct)

    def _add_relative_prefix(self, dct):
        return {'через ' + key: value for key, value in dct.iteritems()}

    def _generate_relative_time(self):
        dct = {}
        for m in xrange(1, 201):
            dct[str(m)] = json.dumps({'minutes': m, 'minutes_relative': True})
            for minute_name in _MINUTE_SYNONYMS:
                dct[' '.join([str(m), minute_name])] = json.dumps({'minutes': m, 'minutes_relative': True})
        for h in xrange(0, 25):
            dct[str(h)] = json.dumps({'hours': h, 'hours_relative': True})
            for hour_name in _HOUR_SYNONYMS:
                dct[' '.join([str(h), hour_name])] = json.dumps({'hours': h, 'hours_relative': True})
                for m in xrange(60):
                    for minute_name in _MINUTE_SYNONYMS:
                        dct[' '.join([str(h), hour_name, str(m), minute_name])] = json.dumps(
                            {'hours': h, 'hours_relative': True, 'minutes': m, 'minutes_relative': True}
                        )
        dct['час'] = json.dumps({'hours': 1, 'hours_relative': True})
        dct['четверть часа'] = json.dumps({'minutes': 15, 'minutes_relative': True})
        dct['полчаса'] = json.dumps({'minutes': 30, 'minutes_relative': True})
        dct['пол часа'] = json.dumps({'minutes': 30, 'minutes_relative': True})
        return self._add_relative_prefix(dct)

    def _generate_repetetive_time(self):
        return {}

    def _generate(self):
        dct = {}
        dct.update(self._generate_absolute_time())
        dct.update(self._generate_relative_time())
        dct.update(self._generate_repetetive_time())
        dct = {_strip_repeating_spaces(key): value for key, value in dct.iteritems()}
        return dct
