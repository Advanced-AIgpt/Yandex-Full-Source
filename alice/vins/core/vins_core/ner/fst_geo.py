# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re
import numbers

from vins_core.ner.fst_base import NluFstBaseValue


_normalizer = None

_REGION_TYPE_ID = {
    "continent": 1,
    "country": 3,
    "city": 6,
    "metro_station": 9
}


class NluFstGeo(NluFstBaseValue):
    def _process_value(self, value):
        out = {}
        if not isinstance(value, list):
            return out
        for i in xrange(0, len(value), 2):
            geo_type, geo_value = value[i].rstrip('='), value[i + 1]
            if geo_type in _REGION_TYPE_ID:
                out[geo_type] = {
                    'id': geo_value,
                    'name': self.maps[unicode(geo_value)]
                }
            else:
                if isinstance(geo_value, basestring):
                    geo_value = re.sub(r'\s+', ' ', geo_value.strip())
                elif isinstance(geo_value, numbers.Integral) and geo_type == 'street':
                    geo_value = unicode(geo_value)
                out[geo_type] = geo_value
        return out
