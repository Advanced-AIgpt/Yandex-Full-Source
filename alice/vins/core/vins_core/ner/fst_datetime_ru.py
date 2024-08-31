# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import datetime
import re

from vins_core.ner.fst_base import NluFstBaseValue, V_BEG, V_END

TYPES = {
    'S': 'seconds',
    'M': 'minutes',
    'H': 'hours',
    'd': 'days',
    'w': 'weeks',
    'm': 'months',
    'Y': 'years',
    'W': 'weekday'
}

_DATETIME_ATTRS = {fmt: name.rstrip('s') for fmt, name in TYPES.iteritems()}


class NluFstDatetimeRu(NluFstBaseValue):

    _TAPE_PATTERN = r'\d+\s+\w[\+\-]?'

    def parse_value(self, type, value):
        if V_BEG not in value:
            return super(NluFstBaseValue, self).parse_value(type, value)
        else:
            substr = value.strip()

            vb = '\\' + V_BEG
            ve = '\\' + V_END
            # collect output tape text
            output_tape = ''.join(re.findall(r'{0}([^{1}]+){1}'.format(vb, ve), substr))
            values = re.findall(self._TAPE_PATTERN, output_tape)
            # clean output tape unless it is numerical
            substr = re.sub(r'{0}(?!\d+{1})[^{1}]+{1}'.format(vb, ve), '', substr)

            # remove remaining brackets
            substr = re.sub(r'[{0}{1}]'.format(vb, ve), '', substr)

            value = self.to_datetime(values)

            return value, substr, None

    def to_datetime(self, values):
        out = {}
        rel_out = []
        for value in values:
            m_out = re.search(r'(\d+)\s+(\w)([\-\+]?)', value)
            v, f, pm = m_out.group(1), m_out.group(2), m_out.group(3)
            v = int(v)
            if pm:
                rel_out.append((f, v if pm == '+' else -v))
            else:
                if f == 'Y' and v / 100 == 0:
                    if v <= 40:
                        # 2 digits are considered as 20xx year
                        v += 2000
                    else:
                        # 2 digits are considered as 19xx year
                        v += 1900
                out[f] = v

        out = self.apply_rel_out(out, rel_out)
        return out

    @staticmethod
    def apply_rel_out(out, whole_rel_out):

        rel_out = [(k, v) for k, v in whole_rel_out if k in out]
        if not rel_out or not out:
            return NluFstDatetimeRu._pretty_out(out, whole_rel_out)

        # fix automata error
        rel_out = list(set(rel_out))  # Does it spoils?
        # rel_out_keys = [i[0] for i in rel_out]
        if ('H', 12) in rel_out and out['H'] >= 12:
            rel_out.remove(('H', 12))
        # ==================
        deltas = [datetime.timedelta(**{TYPES[k]: v}) for k, v in rel_out if k in out]
        td = sum(deltas, datetime.timedelta())
        dateval = []
        for k, v in out.iteritems():
            str_v = str(v)
            if k in ('H', 'M', 'S') and len(str_v) == 1:
                # Fix missed leading '0' for proper strptime formatting
                str_v = '0' + str_v
            dateval.append(str_v)
        dateval = ' '.join(dateval)
        datefmt = ' '.join('%%%s' % fmt for fmt in out.keys())
        dt = datetime.datetime.strptime(dateval, datefmt)

        dt += td

        for out_key in out.keys():
            if out_key == 'W':
                continue
            out[out_key] = NluFstDatetimeRu._get_datetime_attr_by_fmt(dt, out_key)

        rel_out = [(k, v) for k, v in whole_rel_out if k not in out]

        return NluFstDatetimeRu._pretty_out(out, rel_out)

    @staticmethod
    def _get_datetime_attr_by_fmt(datetime_obj, fmt):
        return getattr(datetime_obj, _DATETIME_ATTRS[fmt])

    @staticmethod
    def _pretty_out(out, rel_out):

        pout = {}
        for k, v in out.iteritems():
            pout[TYPES[k]] = v

        for k, v in rel_out:
            if TYPES[k] in pout:
                pout[TYPES[k]] += v
            else:
                pout[TYPES[k]] = v
            pout[TYPES[k] + '_relative'] = True

        if all(['hours_relative' in pout,
                'minutes_relative' in pout,
                'seconds_relative' in pout]):
            pout['time_relative'] = True
            del pout['hours_relative']
            del pout['minutes_relative']
            del pout['seconds_relative']

        if all(['days_relative' in pout,
                'months_relative' in pout,
                'years_relative' in pout]):
            pout['date_relative'] = True
            del pout['days_relative']
            del pout['months_relative']
            del pout['years_relative']

        return pout
