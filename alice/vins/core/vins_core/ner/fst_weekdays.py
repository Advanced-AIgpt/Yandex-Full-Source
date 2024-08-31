# coding: utf-8

from __future__ import unicode_literals

import re

from vins_core.ner.fst_base import NluFstBaseValue, V_BEG, V_END, V_INSERTED

WEEKDAYS = ('1', '2', '3', '4', '5', '6', '7')

TYPE_NAME = 'WEEKDAYS'


class NluFstWeekdays(NluFstBaseValue):
    # TODO: Refactor to use _process_value
    def parse_value(self, type, value):
        if type != TYPE_NAME:
            return super(NluFstBaseValue, self).parse_value(type, value)
        else:
            substr = value.strip()

            vb = '\\' + V_BEG
            ve = '\\' + V_END
            values = re.findall(r'{0}{2}([^{1}]+){1}'.format(vb, ve, V_INSERTED), substr)
            substr = re.sub(r'{0}([^{1}]+){1}'.format(vb, ve), '', substr)

            weekdays = set()
            repeat = False
            is_range = False

            prev_day = 1

            for value in values:
                for v in value:
                    if v in WEEKDAYS:
                        v = ord(v) - ord('1') + 1
                        if is_range:
                            if prev_day < v:
                                weekdays.update(range(prev_day, v + 1))
                            else:
                                weekdays.update(range(prev_day, 7 + 1))
                                weekdays.update(range(1, v + 1))
                            is_range = False
                        else:
                            weekdays.add(v)
                        prev_day = v
                    elif v == '-':
                        is_range = True
                    elif v == '+':
                        repeat = True
            if is_range:
                weekdays.update(range(prev_day, 8))

            return {'weekdays': list(weekdays), 'repeat': repeat}, substr, None
