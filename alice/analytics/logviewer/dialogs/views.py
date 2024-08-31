# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import json
from datetime import datetime, timedelta

from django.http import HttpResponse
from django.shortcuts import render
from django.views.generic import View
from django.core.exceptions import ValidationError

from utils.clickhouse.requesting import post_req
from utils.statface.dicts import StatDict

from models import VoiceLogSelector


def add_empty_dates(counts, st, et):
    date_format = "%Y-%m-%d %H:%M"
    start = datetime.strptime(st, date_format)
    end = datetime.strptime(et, date_format)
    for n in range(int((end - start).days) + 1):
        date = (start + timedelta(n)).strftime("%Y-%m-%d")
        if date not in [d['fielddate'] for d in counts['data']]:
            counts['data'].append({u'fielddate': date, u'cnt': u'0'})
    counts['data'].sort(key=lambda d: d['fielddate'])


class DialogsSampleView(View):
    "Отображение таблицы с выборкой"
    template = 'dialogs/sample.html'

    def get(self, request):
        return render(request, self.template,
                      context={'default_date': VoiceLogSelector.get_default_date()})


class DialogsQueryView(View):
    "Данные по запросу"
    FIELDS = ['st', 'et', 'uuid', 'app', 'query', 'reply', 'intent', 'skill_id', 'stats_daily']

    def get(self, request):
        unknown = set(request.GET).difference(self.FIELDS)
        if unknown:
            if len(unknown) == 1:
                msg = 'Незнакомый параметр: %s' % ', '.join(unknown)
            else:
                msg = 'Незнакомые параметры: %s' % ', '.join(unknown)

            return HttpResponse(status=400,
                                content=json.dumps({'all': [msg]}),
                                content_type='application/json')

        get = request.GET.get
        kwargs = {field: get(field) for field in self.FIELDS if field != 'stats_daily'}
        try:
            selector = VoiceLogSelector(**kwargs)
        except ValidationError, err:
            return HttpResponse(status=400,
                                content=json.dumps(err.message_dict),
                                content_type='application/json')

        sample_table = json.loads(post_req(selector.get_query()))

        counts = None
        if get('stats_daily') == '1':
            counts = json.loads(post_req(selector.get_count_daily_query()))
            add_empty_dates(counts, kwargs['st'], kwargs['et'])

        total = json.loads(post_req(selector.get_count_total_query()))['data'][0]['cnt']
        response = {
            'sample_table': sample_table,
            'counts': counts,
            'total': total
        }
        return HttpResponse(content=json.dumps(response), content_type='application/json')


class DialogsExtSkillsView(View):
    """Словарь скилов"""
    sd = StatDict('external_skills_pa')

    def get(self, request):
        dct = self.sd.content
        if not dct and self.sd.last_err:
            return HttpResponse(self.sd.last_err, status=502)
        return HttpResponse(json.dumps(dct), content_type='application/json')
