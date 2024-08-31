# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.fst_utils import fstring
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor


class NluFstDatetimeTrConstructor(NluFstBaseValueConstructor):
    def create(self):

        super(NluFstDatetimeTrConstructor, self).create()

        f_spec_day = self.spec_day()
        f_relative = gn.Fst.union_seq([
            f_spec_day
        ])

        self.fsts = [
            self.ftag + f_relative,
        ]

    def spec_day(self):
        now_synonims = [
            fstring('şimdi'), fstring('mevcut')
        ]
        return gn.Fst.union_seq([
            gn.Fst.union_seq(
                [fstring('önceki gün')]
            ) + self.vb + gn.insert('2 d-') + self.ve,
            gn.Fst.union_seq(
                ['dün']
            ) + self.vb + gn.insert('1 d-') + self.ve,
            gn.Fst.union_seq(
                ['bugün', fstring('gün boyunca')]
            ) + self.vb + gn.insert('0 d+') + self.ve,
            gn.Fst.union_seq(now_synonims) + self.vb + gn.insert('0 S+') + self.ve,
            gn.Fst.union_seq(
                ['yarın']
            ) + self.vb + gn.insert('1 d+') + self.ve,
            gn.Fst.union_seq(
                [fstring('yarından sonraki gün'), fstring('öbür gün')]
            ) + self.vb + gn.insert('2 d+') + self.ve,
        ])
