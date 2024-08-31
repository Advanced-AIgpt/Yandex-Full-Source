# coding: utf-8
from __future__ import unicode_literals

from vins_core.ner.fst_float import NluFstFloat


class NluFstUnits(NluFstFloat):
    def _process_value(self, value):
        if not isinstance(value, list):
            return value
        return {units_name: float(num) for num, units_name in [value[i:i + 2] for i in xrange(0, len(value), 2)]}
