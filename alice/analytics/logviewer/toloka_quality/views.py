# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import json

from django.http import HttpResponse
from django.shortcuts import render
from django.views.generic import View
from django.core.exceptions import ValidationError

from utils.clickhouse.requesting import post_req

from toloka_quality.models import TolokaQualityLogSelector


class TolokaQualitySampleView(View):
    "Отображение таблицы с выборкой"
    template = 'toloka_quality/sample.html'

    def get(self, request):
        default_start_date, default_end_date = TolokaQualityLogSelector.get_default_date()
        return render(request, self.template,
                      context={
                          'default_start_date': default_start_date,
                          'default_end_date': default_end_date,
                      })


class TolokaQualityQueryView(View):
    "Данные по запросу"
    FIELDS = ['sd', 'ed', 'uuid', 'url', 'app', 'context_0', 'context_1', 'context_2', 'reply', 'action', 'intent']

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
        kwargs = {field: get(field) for field  in self.FIELDS}
        try:
            selector = TolokaQualityLogSelector(**kwargs)
        except ValidationError, err:
            return HttpResponse(status=400,
                                content=json.dumps(err.message_dict),
                                content_type='application/json')

        response = post_req(selector.get_query())
        return HttpResponse(content=response, content_type='application/json')
