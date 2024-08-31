# coding: utf-8
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.ru.fst_datetime import NluFstDatetimeRuConstructor


class NluFstDateRuConstructor(NluFstDatetimeRuConstructor):
    def units(self, sign='', filter_units=None):
        return super(NluFstDateRuConstructor, self).units(sign, filter_units=list('dwmYW'))

    def create(self):

        super(NluFstDateRuConstructor, self).create()

        f_forward = self.forward([
            self.funits_plus,
        ])
        f_backward = self.backward([
            self.funits_minus,
        ])
        f_spec_day = self.spec_day() | self.weekday()

        f_relative = gn.Fst.union_seq([
            f_forward,
            f_backward,
            f_spec_day
        ])
        self.fsts = [
            self.ftag + f_relative,
            self.ftag + self._date
        ]
