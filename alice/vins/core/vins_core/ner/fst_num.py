# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.ner.fst_base import NluFstBaseValue


class NluFstNum(NluFstBaseValue):
    def parse_value(self, type, value):
        value, substr, w = super(NluFstNum, self).parse_value(type, value)
        if isinstance(value, (list, tuple)) and len(value) == 2 and value[0] == '-':
            # negate consequent number
            value = -value[1]
        return value, substr, w
