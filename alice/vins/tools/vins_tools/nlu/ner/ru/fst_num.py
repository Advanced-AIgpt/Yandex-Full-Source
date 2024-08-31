# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases, ordinal_suffix


class NluFstNumRuConstructor(NluFstBaseValueConstructor):
    def create(self):
        super(NluFstNumRuConstructor, self).create()

        self.fsts = [
            self.ftag + self.num(),
            self.ftag + self.ordinal(),
            self.ftag + self.nouns()
        ]

    def num(self, include_negatives=True):
        num = gn.pp(gn.g.digit)
        if include_negatives:
            return self._catch(gn.qq('-') + num) | gn.Fst('минус') + self.w + self._insert('-') + self._catch(num)
        return self._catch(num)

    def ordinal(self):
        num = self.num()
        return num + ordinal_suffix()

    def nouns(self):
        return gn.Fst.union_seq([
            fst_cases('нолик') + self._insert('0'),
            fst_cases('зеро') + self._insert('0'),
            fst_cases('zero') + self._insert('0'),
            fst_cases('единица') + self._insert('1'),
            fst_cases('единичка') + self._insert('1'),
            fst_cases('однерка') + self._insert('1'),
            fst_cases('однерочка') + self._insert('1'),
            fst_cases('двойка') + self._insert('2'),
            fst_cases('двоечка') + self._insert('2'),
            fst_cases('пара') + self._insert('2'),
            fst_cases('тройка') + self._insert('3'),
            fst_cases('троечка') + self._insert('3'),
            fst_cases('тройбас') + self._insert('3'),
            fst_cases('четверка') + self._insert('4'),
            fst_cases('четверочка') + self._insert('4'),
            fst_cases('пятерка') + self._insert('5'),
            fst_cases('пятерочка') + self._insert('5'),
            fst_cases('пятак') + self._insert('5'),
            fst_cases('шестерка') + self._insert('6'),
            fst_cases('шестерочка') + self._insert('6'),
            fst_cases('семерка') + self._insert('7'),
            fst_cases('семерочка') + self._insert('7'),
            fst_cases('восьмерка') + self._insert('8'),
            fst_cases('восьмерочка') + self._insert('8'),
            fst_cases('девятка') + self._insert('9'),
            fst_cases('девяточка') + self._insert('9'),
            fst_cases('десятка') + self._insert('10'),
            fst_cases('десяточка') + self._insert('10'),
        ])
