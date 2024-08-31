# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.ner.fst_num import NluFstNum
from vins_core.ner.fst_float import NluFstFloat


class NluFstCalc(NluFstFloat, NluFstNum):
    def parse_value(self, type, value):
        return value, value.strip(), None
