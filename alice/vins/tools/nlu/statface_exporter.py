#!/usr/bin/env python
# coding: utf-8

from __future__ import unicode_literals

import json
import logging

from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting
from vins_core.utils.datetime import utcnow

STATFACE_AUTH_TOKEN = get_setting('STATFACE_AUTH_TOKEN')

logger = logging.getLogger(__name__)


class SimpleStatfaceExporter(BaseHTTPAPI):
    _url_prefix = 'https://upload.stat.yandex-team.ru/_api/report'
    _headers = {
        'Authorization': 'OAuth %s' % STATFACE_AUTH_TOKEN
    }

    def __init__(self):
        super(SimpleStatfaceExporter, self).__init__(headers=self._headers, timeout=120)

    def export_config(self, name, string_dimensions, measure_names, title):
        report_config = {
            'dimensions': [
                {'fielddate': 'date'},
            ] + [{d: 'string'} for d in string_dimensions],
            'measures': [{m: 'number'} for m in measure_names]
        }

        json_config = json.dumps(
            {
                'user_config': report_config,
                'title': title,
            }
        )
        logging.info('Exporting config data to report %s, data: %s' % (name, json_config))

        resp = self.post(
            '%s/config' % self._url_prefix,
            data={
                'json_config': json_config,
                'name': name,
                'scale': 'd'
            }
        )

        resp.raise_for_status()

    def export_data(self, name, data_collection, date=None):
        if not date:
            date = utcnow()

        json_data = json.dumps({
            "values": [
                dict({'fielddate': date.strftime('%Y-%m-%d')}, **data)
                for data in data_collection
            ]
        })
        logging.info('Exporting data to report %s, data: %s' % (name, json_data))
        resp = self.post(
            '%s/data' % self._url_prefix,
            data={
                'json_data': json_data,
                'name': name,
                'scale': 'd'
            }
        )

        resp.raise_for_status()

    def remove_report(self, name):
        logging.info('Removing report %s' % name)

        resp = self.post(
            '%s/delete_report' % self._url_prefix,
            data={
                'name': name,
            },
        )

        resp.raise_for_status()
