# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re
from datetime import datetime
from time import mktime, strptime


class BaseLogSelector(object):
    def rx_clean(self, field, value, err_dict, rx, msg):
        if value and value.strip():
            value = value.strip()
            if not re.match(rx, value):
                err_dict[field] = [msg]
        return value

    def timespan_clean(self, st, et, err_dict):
        if st:
            st = st.strip()
        else:
            st = '%s 00:00' % self.get_default_date()

        if et:
            et = et.strip()
        else:
            et = '%s 23:59' % self.get_default_date()

        try:
            st = int(mktime(strptime(st, '%Y-%m-%d %H:%M')))
        except ValueError, err:
            err_dict['st'] = ['Время начала должно быть в формате YYYY-MM-DD hh:mm']

        try:
            et = int(mktime(strptime(et, '%Y-%m-%d %H:%M'))) + 59
            # захватываем всю указанную минуту, а не только её нулевую секунду
        except ValueError, err:
            err_dict['et'] = ['Время окончания должно быть в формате YYYY-MM-DD hh:mm']
        else:
            if isinstance(st, int) and et < st:
                err_dict['et'] = ['Время окончания не должно быть раньше времени начала']
            # TODO: Проверку на наличие в базе запрошенных периодов

        return st, et

    def datespan_clean(self, sd, ed, err_dict):
        if sd:
            sd = sd.strip()
        else:
            sd = self.get_default_date()

        if ed:
            ed = ed.strip()
        else:
            ed = self.get_default_date()

        try:
            sd = int(mktime(strptime(sd, '%Y-%m-%d')))
        except ValueError, err:
            err_dict['sd'] = ['Дата начала должна быть в формате YYYY-MM-DD']

        try:
            ed = int(mktime(strptime(ed, '%Y-%m-%d')))
        except ValueError, err:
            err_dict['et'] = ['Дата окончания должна быть в формате YYYY-MM-DD']
        else:
            if isinstance(sd, int) and ed < sd:
                err_dict['et'] = ['Дата окончания не должна быть раньше даты начала']

        return sd, ed

    is_raw_pattern = re.compile('^[\w\s-]+$', flags=re.U).match
    fmt_like = "positionCaseInsensitiveUTF8({}, '{}') > 0".format
    fmt_rx = "match(lowerUTF8({}), '{}')".format

    def make_mask_cond(self, field):
        pattern = getattr(self, field)
        if self.is_raw_pattern(pattern):
            return self.fmt_like(field, pattern)
        else:
            return self.fmt_rx(field, pattern.lower())

    def time_conditions(self, st, et):
        from_date, from_time = datetime.fromtimestamp(st).strftime('%Y-%m-%d %H:%M').split()
        to_date, to_time = datetime.fromtimestamp(et).strftime('%Y-%m-%d %H:%M').split()
        if to_time == '00:00':  # Другой способ указать границу дат
            to_date, to_time = datetime.fromtimestamp(et - 60).strftime('%Y-%m-%d %H:%M').split()

        if from_date == to_date:
            yield "fielddate = '{}'".format(from_date)
        else:
            yield "fielddate >= '{}'".format(from_date)
            yield "fielddate <= '{}'".format(to_date)

        if from_time != '00:00':
            yield 'ts >= {}'.format(st)

        if to_time != '23:59':
            yield 'ts <= {}'.format(et)

    def date_conditions(self, sd, ed):
        from_date = datetime.fromtimestamp(sd).strftime('%Y-%m-%d')
        to_date = datetime.fromtimestamp(ed).strftime('%Y-%m-%d')

        if from_date == to_date:
            yield "fielddate = '{}'".format(from_date)
        else:
            yield "fielddate >= '{}'".format(from_date)
            yield "fielddate <= '{}'".format(to_date)

    def get_columns(self):
        raise NotImplementedError('get_mask_fields function is not implemented')

    def get_conditions(self):
        raise NotImplementedError('get_conditions function is not implemented')

    raw_query_fmt = """
        SELECT {columns}
        FROM {table}
        WHERE {conditions}
        LIMIT 500
        FORMAT JSON
    """.format
