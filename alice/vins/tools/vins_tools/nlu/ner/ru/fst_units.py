# coding: utf-8
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.ru.fst_float import NluFstFloatRuConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases, fst_cases_num


class NluFstUnitsRuConstructor(NluFstFloatRuConstructor):
    def _floats(self):
        return self._catch(gn.pp(gn.g.digit) | self._float | self._fractions)

    def create(self):
        super(NluFstUnitsRuConstructor, self).create()

        floats = self._floats()

        unit = self._get_units(pluralize=False)
        units = self._get_units(pluralize=True)
        num_units = floats + self.w + units
        one_unit = self._insert('1') + unit
        two_units = fst_cases('пара') + self._insert('2') + self.w + units
        half_unit = (gn.Fst('пол') | fst_cases('половина') + self.w) + self._insert('0.5') + unit
        quarter_units = fst_cases('четверть') + self._insert('0.25') + self.w + unit
        one_and_half_units = gn.Fst('полтор' + gn.anyof('ыа')) + self._insert('1.5') + self.w + unit

        all_units = num_units | one_unit | two_units | half_unit | quarter_units | one_and_half_units
        sep = gn.qq(gn.Fst.union_seq([
            ',', self.w + 'и'
        ])) + self.w

        loop_units = all_units + gn.ss(sep + all_units)

        self.fsts = [
            self.ftag + loop_units
        ]

    def _get_units(self, pluralize):
        raise NotImplementedError()


class NluFstUnitsTimeRuConstructor(NluFstUnitsRuConstructor):
    def create(self):
        super(NluFstUnitsTimeRuConstructor, self).create()

        one_minute = self._insert('1') + fst_cases('минута') + self._insert('minutes')
        one_hour = self._insert('1') + fst_cases('час') + self._insert('hours')

        unit_and_float = gn.Fst.union_seq((
            one_minute + self.w + self._floats() + self._insert('seconds'),
            one_hour + self.w + self._floats() + self._insert('minutes'),
        ))

        only_floats = (
            self._floats() + self._insert('minutes') + gn.qq(self.w + self._floats() + self._insert('seconds'))
        )

        self.fsts += [
            self.ftag + unit_and_float,
            self.ftag + only_floats,
        ]

    def _get_units(self, pluralize):
        inflect = fst_cases_num if pluralize else fst_cases
        units = gn.Fst.union_seq([
            inflect('секунда') + self._insert('seconds'),
            inflect('минута') + self._insert('minutes'),
            inflect('час') + self._insert('hours')
        ])
        return units
