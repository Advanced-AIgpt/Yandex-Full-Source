# coding: utf-8

from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases, fst_cases_num


class NluFstWeekdaysRuConstructor(NluFstBaseValueConstructor):
    def _gen_weekday_names(self, pluralize=False):
        return gn.Fst.union_seq((
            fst_cases('понедельник', pluralize) + self._insert('1+' if pluralize else '1'),
            fst_cases('вторник', pluralize) + self._insert('2+' if pluralize else '2'),
            fst_cases('среда', pluralize) + self._insert('3+' if pluralize else '3'),
            fst_cases('четверг', pluralize) + self._insert('4+' if pluralize else '4'),
            fst_cases('пятница', pluralize) + self._insert('5+' if pluralize else '5'),
            fst_cases('суббота', pluralize) + self._insert('6+' if pluralize else '6'),
            fst_cases('воскресенье', pluralize) + self._insert('7+' if pluralize else '7'),
        ))

    def create(self):
        super(NluFstWeekdaysRuConstructor, self).create()

        f_ins = gn.Fst.union_seq(('в', 'во', 'на'))
        f_ons = gn.Fst('по')
        f_froms = gn.Fst.union_seq(('с', 'со'))
        f_tos = gn.Fst.union_seq(('по', 'до'))

        f_days_n_ends = gn.Fst.union_seq((
            fst_cases('будни') + self._insert('12345'),
            fst_cases_num('будние') + self._insert('12345'),
            fst_cases_num('рабочие') + self._insert('12345'),
            fst_cases_num('выходные') + self._insert('67'),
        ))

        f_weekdays = self._gen_weekday_names()
        f_weekdays_plur = self._gen_weekday_names(pluralize=True)

        f_in_weekday = f_ins + self.w + (f_weekdays | f_weekdays_plur)
        f_on_weekday = f_ons + self.w + f_weekdays_plur
        f_and = gn.qq(gn.Fst('и') + self.w)
        f_repeat = self._insert('+')
        f_range = self._insert('-')
        f_phrase = gn.Fst.union_seq((
            f_ins + self.w + f_days_n_ends,
            f_ons + self.w + f_days_n_ends + f_repeat,
            f_in_weekday + gn.ss(self.w + f_and + (f_in_weekday | f_weekdays)),
            f_on_weekday + gn.ss(self.w + f_and + (f_on_weekday | f_weekdays_plur)),
            f_froms + self.w + f_weekdays + f_range + gn.qq(self.w + f_tos + self.w + f_weekdays),
        ))

        f_any = gn.Fst.union_seq((fst_cases('каждый'), fst_cases('каждая'), fst_cases('каждое')))
        f_anys = gn.Fst.union_seq((fst_cases_num('каждый'), fst_cases_num('каждая'), fst_cases_num('каждое')))

        f_any_weekday = gn.Fst.union_seq((
            f_any + self.w + f_weekdays + gn.ss(self.w + f_and + f_weekdays) + f_repeat,
            f_anys + self.w + f_days_n_ends + f_repeat,
            f_any + self.w + fst_cases('день') + self._insert('1234567') + f_repeat,
            fst_cases('ежедневно') + self._insert('1234567') + f_repeat,
            fst_cases('ежедневный') + self._insert('1234567') + f_repeat,
            fst_cases('всю неделю') + self._insert('1234567') + f_repeat,
        ))

        self.fsts = [
            self.ftag + f_phrase,
            self.ftag + f_any_weekday,
        ]
