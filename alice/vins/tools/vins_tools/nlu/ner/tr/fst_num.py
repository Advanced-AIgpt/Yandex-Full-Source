# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases


class NluFstNumTrConstructor(NluFstBaseValueConstructor):
    def create(self):
        super(NluFstNumTrConstructor, self).create()

        self.fsts = [
            self.ftag + self.num(),
            self.ftag + self.nouns()
        ]

    def num(self, include_negatives=True):
        num = gn.pp(gn.g.digit)
        if include_negatives:
            return self._catch(gn.qq('-') + num) | gn.Fst('eksi') + self.w + self._insert('-') + self._catch(num)
        return self._catch(num)

    def nouns(self):
        return gn.Fst.union_seq([
            fst_cases('birim', lang='tr') + self._insert('1'),
            fst_cases('ikili', lang='tr') + self._insert('2'),
            fst_cases('bir çift', lang='tr') + self._insert('2'),
            fst_cases('üçlü', lang='tr') + self._insert('3'),
            fst_cases('dört', lang='tr') + self._insert('4'),
            fst_cases('beş', lang='tr') + self._insert('5'),
            fst_cases('altı', lang='tr') + self._insert('6'),
            fst_cases('yedi', lang='tr') + self._insert('7'),
            fst_cases('sekiz', lang='tr') + self._insert('8'),
            fst_cases('dokuz', lang='tr') + self._insert('9'),
            fst_cases('bir düzine', lang='tr') + self._insert('10'),
        ])
