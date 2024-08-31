# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_core.ner.fst_float import FRACTION_SYM
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor


class NluFstFloatRuConstructor(NluFstBaseValueConstructor):
    def create(self):

        super(NluFstFloatRuConstructor, self).create()

        self._float = self.float()
        self._fractions = self.fractions()
        self._int_part_frac_part = self.int_part_frac_part()
        self._int_and_int = self.int_and_int()

        self.fsts = [
            self.ftag + self._catch(self._float),
            self.ftag + self._catch(self._fractions),
            self.ftag + self._catch(self._int_part_frac_part),
            self.ftag + self._catch(self._int_and_int)
        ]

    def float(self):
        return gn.qq('-') + gn.pp(gn.g.digit) + gn.anyof('.,') + gn.ss(gn.g.digit)

    def fractions(self):
        return (
            gn.qq('-') + gn.pp(gn.g.digit) + gn.qq(self.w) +
            FRACTION_SYM + gn.qq(self.w) + (
                gn.anyof('23456789') + gn.ss(gn.g.digit) |
                gn.Fst('1') + gn.pp(gn.g.digit)
            )
        )

    def int_part_frac_part(self):
        # три пятьдесят
        int_part = gn.g.digit
        frac_part = gn.anyof('123456789') + '0'
        return int_part + self.w + frac_part

    def int_and_int(self):
        # 100 и 7 -> 100.7
        int = gn.pp(gn.g.digit)
        return int + self.w + 'и' + self.w + int
