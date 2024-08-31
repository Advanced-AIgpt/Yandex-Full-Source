# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re

from vins_core.ner.fst_base import NluFstBase


class NluFstCustom(NluFstBase):
    @staticmethod
    def normalize(text):
        return re.sub(r'\s+', ' ', text).strip()

    def parse_value(self, type, value):
        nvalue = self.normalize(value)
        if nvalue in self.maps:
            canon = self.maps[nvalue]
            return canon, nvalue, self.weights.get(canon, {}).get(nvalue)
        else:
            return super(NluFstCustom, self).parse_value(type, nvalue)


class NluFstCustomHierarchy(NluFstCustom):
    def parse_value(self, type, value):
        nvalue = self.normalize(value)
        if not type:
            return super(NluFstCustom, self).parse_value(type, nvalue)
        if type not in self.maps or nvalue not in self.maps[type]:
            return super(NluFstCustom, self).parse_value(type, nvalue)
        canon = self.maps[type][nvalue]
        return canon, nvalue, self.weights.get(type + '_' + canon, {}).get(nvalue)
