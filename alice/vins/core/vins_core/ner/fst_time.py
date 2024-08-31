# coding: utf-8
from __future__ import unicode_literals

import re

from vins_core.ner.fst_base import NluFstBaseValue
from vins_core.ner.fst_datetime_ru import NluFstDatetimeRu


class NluFstTime(NluFstBaseValue):
    _UNIT_PATTERN = r'(?P<num>\d+)|(?P<format>[SMH])(?P<sign>[\-\+]?)|(?P<period>am|pm|morning|day|evening|night)'

    def _process_value(self, value):
        if not isinstance(value, list):
            return value
        out, rel_out = {}, []
        num, period = None, None
        for item in value:
            m = re.match(self._UNIT_PATTERN, unicode(item)).groupdict()
            if m['format']:
                if num is None:
                    raise ValueError('Output tape is wrong: "format" field specified before "num"')
                num = int(num)
                if m['sign']:
                    rel_out.append((m['format'], num if m['sign'] == '+' else -num))
                else:
                    out[m['format']] = num
            num = m['num']
            if m['period']:
                period = m['period']
        out = NluFstDatetimeRu.apply_rel_out(out, rel_out)
        if period:
            # 0:30am should be 12:30am
            if out.get('hours') == 0:
                out['hours'] = 12
            out['period'] = self._normalize_period(period, out.get('hours', 0))
        return out

    @staticmethod
    def _normalize_period(period, hours):
        # There is nothing special in constant 7. It's just my perception of day and night.
        if period == 'morning':
            return 'pm' if hours == 12 else 'am'
        if period == 'day':
            return 'pm' if (hours == 12 or hours <= 7) else 'am'
        if period == 'evening':
            return 'am' if hours == 12 else 'pm'
        if period == 'night':
            return 'am' if (hours == 12 or hours <= 7) else 'pm'
        return period
