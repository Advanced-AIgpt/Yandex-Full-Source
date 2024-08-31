# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.core.exceptions import ValidationError
from datetime import datetime, timedelta

from logviewer.base_models import BaseLogSelector


class VoiceLogSelector(BaseLogSelector):
    def __init__(self, st=None, et=None,
                 uuid=None, app=None,
                 query=None, reply=None,
                 intent=None, skill_id=None):
        err_dict = {}

        self.st, self.et = self.timespan_clean(st, et, err_dict)

        self.uuid = self.rx_clean('uuid', uuid, err_dict, r'^[\w/-]+$',
                                  'uuid должен состоять только из латиницы, дефисов и прямого слеша')

        self.app = self.rx_clean('app', app, err_dict, r'^\w+$',
                                 'app должен состоять только из латинских буквоцифр и подчёркиваний')

        self.query = query
        self.reply = reply
        self.intent = intent

        self.skill_id = self.rx_clean('skill_id', skill_id, err_dict, r'^[\w-]+$',
                                      'skill_id должен состоять только из латиницы и дефисов')

        self.table = 'analytics.dialogs'

        if err_dict:
            raise ValidationError(err_dict)

    EXACT_FIELDS = ['uuid', 'app', 'skill_id']
    MASK_FIELDS = ['query', 'reply', 'intent']

    @staticmethod
    def get_default_date():
        return (datetime.now() - timedelta(days=1, hours=4)).strftime('%Y-%m-%d')

    def get_conditions(self):
        conditions = ["{} = '{}'".format(f, getattr(self, f))
                      for f in self.EXACT_FIELDS
                      if getattr(self, f)]

        conditions.extend(self.make_mask_cond(f)
                          for f in self.MASK_FIELDS
                          if getattr(self, f))

        conditions.extend(self.time_conditions(self.st, self.et))

        return ' AND '.join(conditions)

    def get_columns(self):
        columns = ['ts', 'uuid', 'req_id', 'query', 'reply', 'app', 'intent', 'gc_intent', 'skill_id']
        return ', '.join(columns)

    def get_query(self):
        return self.raw_query_fmt(conditions=self.get_conditions(),
                                  columns=self.get_columns(),
                                  table=self.table)

    def get_count_daily_query(self):
        return """
            SELECT fielddate, count(*) AS cnt
            FROM {table}
            WHERE {conditions}
            GROUP BY fielddate
            ORDER BY fielddate
            FORMAT JSON
        """.format(
            table=self.table,
            conditions=self.get_conditions()
        )

    def get_count_total_query(self):
        return """
            SELECT count(*) AS cnt
            FROM {table}
            WHERE {conditions}
            FORMAT JSON
        """.format(
            table=self.table,
            conditions=self.get_conditions()
        )
