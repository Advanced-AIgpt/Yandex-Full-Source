# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from datetime import datetime, timedelta
from django.core.exceptions import ValidationError

from logviewer.base_models import BaseLogSelector


class TolokaQualityLogSelector(BaseLogSelector):
    def __init__(self, sd=None, ed=None,
                 uuid=None, app=None,
                 context_0=None, context_1=None,
                 context_2=None, reply=None,
                 action=None, intent=None,
                 url=None):
        err_dict = {}

        self.sd, self.ed = self.datespan_clean(sd, ed, err_dict)

        self.uuid = self.rx_clean('uuid', uuid, err_dict, r'^[\w/-]+$',
                                  'uuid должен состоять только из латиницы, дефисов и прямого слеша')

        self.app = self.rx_clean('app', app, err_dict, r'^\w+$',
                                 'app должен состоять только из латинских буквоцифр и подчёркиваний')

        self.context_0 = context_0
        self.context_1 = context_1
        self.context_2 = context_2
        self.reply = reply
        self.action = action
        self.intent = intent
        self.url = url

        if err_dict:
            raise ValidationError(err_dict)

    EXACT_FIELDS = ['uuid', 'app']
    MASK_FIELDS = ['context_0', 'context_1', 'context_2', 'reply', 'action', 'intent']

    @staticmethod
    def get_default_date():
        default_start_date = (datetime.now() - timedelta(days=30, hours=4)).strftime('%Y-%m-%d')
        default_end_date = (datetime.now()).strftime('%Y-%m-%d')
        return default_start_date, default_end_date

    def get_conditions(self):
        conditions = ["{} = '{}'".format(f, getattr(self, f))
                      for f in self.EXACT_FIELDS
                      if getattr(self, f)]

        conditions.extend(self.make_mask_cond(f)
                          for f in self.MASK_FIELDS
                          if getattr(self, f))

        conditions.extend(self.date_conditions(self.sd, self.ed))

        return ' AND '.join(conditions)

    def get_columns(self):
        columns = ['url', 'fielddate', 'uuid', 'req_id', 'context_0', 'context_1', 'context_2', 'reply', 'action', 'app', 'intent']
        return ', '.join(columns)

    def get_query(self):
        return self.raw_query_fmt(conditions=self.get_conditions(),
                                  columns=self.get_columns(),
                                  table='analytics.toloka_quality')
