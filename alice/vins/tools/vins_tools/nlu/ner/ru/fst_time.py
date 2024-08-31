# coding: utf-8
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases_num, fst_cases, fstring


class NluFstTimeRuConstructor(NluFstBaseValueConstructor):
    def create(self):
        super(NluFstTimeRuConstructor, self).create()

        time_of_day = self._time_of_day()
        time_with_hours = gn.Fst.union_seq([
            self._catch(self.num_hours_24()) + self._insert('H'),
            self._hours(),
            self._hours_and_some_part(),
            self._hours_and_minutes(),
            self._hours_minutes_seconds(),
            self._half_past(),
            self._minute_past(),
            self._minute_to(),
            self._noon_midnight(),
            self._hhmm(),
        ])

        time_with_hours = gn.Fst.union_seq([
            time_of_day + self.w + self.prep() + self.w + time_with_hours,
            time_with_hours + gn.qq(self.w + time_of_day)
        ])

        self.fsts = [
            self.ftag + self._seconds(),
            self.ftag + self._minutes(),
            self.ftag + self._minutes_and_seconds(),
            self.ftag + self._minutes_and_some_part(),

            self.ftag + self._time_of_day_in_middle(),
            self.ftag + time_with_hours,

            self.ftag + self._relative_forward(),
            self.ftag + self._relative_backward(),
        ]

    @staticmethod
    def prep():
        fp = gn.Fst.union_seq([
            'приблизительно',
            'примерно'
        ])
        return gn.qq(fp + gn.pp(' ')) + gn.anyof([
            'в', 'на', 'к', 'до'
            'перед', 'после', 'около'
        ])

    @staticmethod
    def seconds_noun(include_plur=True):
        return (fst_cases_num('секунда') | fst_cases_num('секундка') | fst_cases_num('секундочка')) \
            if include_plur else (fst_cases('секунда') | fst_cases('секундка') | fst_cases('секундочка'))

    @staticmethod
    def num_seconds():
        return gn.g.digit | (gn.anyof('012345') + gn.g.digit)

    @staticmethod
    def minutes_noun(include_plur=True):
        return (fst_cases_num('минута') | fst_cases_num('минутка') | fst_cases_num('минуточка')) \
            if include_plur else (fst_cases('минута') | fst_cases('минутка') | fst_cases('минуточка'))

    @staticmethod
    def num_minutes():
        return gn.g.digit | (gn.anyof('012345') + gn.g.digit)

    @staticmethod
    def hours_noun(include_plur=True):
        return (fst_cases_num('час') | fst_cases_num('часик') | fst_cases_num('часок') | fst_cases_num('часочек')) \
            if include_plur else (fst_cases('час') | fst_cases('часик') | fst_cases('часок') | fst_cases('часочек'))

    @staticmethod
    def num_hours_12(include_zero=True):
        d = gn.g.digit if include_zero else gn.anyof('123456789')
        return d | gn.Fst.union_seq(['10', '11', '12'])

    @staticmethod
    def num_hours_24(include_24=True):
        hours_2x = gn.Fst('2') + gn.anyof('0123')
        if include_24:
            hours_2x |= '24'
        return gn.g.digit | (gn.Fst('1') + gn.g.digit) | hours_2x

    def _seconds(self, insert_suffix=''):
        one = self.seconds_noun(False) + self._insert_seq('1', 'S%s' % insert_suffix)
        two = fst_cases('пара') + self._insert_seq('2', 'S%s' % insert_suffix) + self.w + self.seconds_noun()
        many = self._catch(self.num_seconds()) + self._insert_seq('S%s' % insert_suffix) + self.w + self.seconds_noun()
        return gn.Fst.union_seq([one, two, many])

    def _minutes(self, insert_suffix=''):
        half = gn.Fst('полминут') + gn.Fst.union_seq(['ы', 'ки', 'очки']) + self._insert_seq('30', 'S%s' % insert_suffix)
        one = self.minutes_noun(False) + self._insert_seq('1', 'M%s' % insert_suffix)
        one_and_half = gn.Fst('полторы') + self._insert_seq(
            '1', 'M%s' % insert_suffix) + self._insert_seq(
            '30', 'S%s' % insert_suffix) + self.w + self.minutes_noun(False)
        two = fst_cases('пара') + self._insert_seq('2', 'M%s' % insert_suffix) + self.w + self.minutes_noun()
        many = self._catch(self.num_minutes()) + self._insert('M%s' % insert_suffix) + self.w + self.minutes_noun()
        many_and_some_part = self._minutes_and_some_part(insert_suffix)
        return gn.Fst.union_seq([half, one, one_and_half, two, many, many_and_some_part])

    def _hours(self, insert_suffix=''):
        half_hour = gn.Fst.union_seq([gn.Fst('полчас'), gn.Fst('пол') + self.w + gn.Fst('час')])
        half = half_hour + gn.Fst.union_seq(['а', 'ика']) + self._insert_seq('30', 'M%s' % insert_suffix)
        one = self.hours_noun(False) + self._insert_seq('1', 'H%s' % insert_suffix)
        one_and_half = gn.Fst('полтора') + self._insert_seq(
            '1', 'H%s' % insert_suffix) + self._insert_seq(
            '30', 'M%s' % insert_suffix) + self.w + self.hours_noun(False)
        two = fst_cases('пара') + self._insert_seq('2', 'H%s' % insert_suffix) + self.w + self.hours_noun()
        many = self._catch(self.num_hours_24()) + self._insert('H%s' % insert_suffix) + self.w + self.hours_noun()
        many_and_some_part = self._hours_and_some_part(insert_suffix)
        return gn.Fst.union_seq([half, one, one_and_half, two, many, many_and_some_part])

    def _minute_to(self):
        # без 5 минут 6
        minutes = gn.Fst.union_seq([
            self._catch(self.num_minutes()) + self._insert_seq('M-', '0', 'M'),
            gn.Fst('четверти') + self._insert_seq('15', 'M-', '0', 'M'),
            gn.Fst('половины') + self._insert_seq('30', 'M-', '0', 'M')
        ]) + gn.qq(self.w + self.minutes_noun())
        hours = gn.Fst.union_seq([
            self._catch(self.num_hours_24()) + self._insert('H') + gn.qq(gn.Fst(':00') | self.hours_noun()),
            gn.Fst('час') + self._insert_seq('1', 'H'),
        ])
        return gn.Fst('без') + self.w + minutes + self.w + hours

    def _minute_past(self):
        # половина первого
        minutes = gn.Fst.union_seq([
            self._catch(self.num_minutes()) + self._insert_seq(
                'M+', '0', 'M', '1', 'H-') + self.w + self.minutes_noun(),
            fst_cases('четверть') + self._insert_seq('15', 'M+', '0', 'M', '1', 'H-'),
            gn.Fst.union_seq(['пол', fst_cases('половина')]) + self._insert_seq('30', 'M+', '0', 'M', '1', 'H-')
        ])
        hours = self._catch(self.num_hours_12(include_zero=False)) + self._insert('H')
        # TODO: remove qq after DIALOG-1324
        hours += gn.qq('-го')
        return minutes + self.w + hours

    def _hhmm(self):
        hours = self._catch(self.num_hours_24()) + self._insert('H')
        minutes = self._catch(self.num_minutes()) + self._insert('M')
        return hours + ':' + minutes

    def _half_past(self):
        return gn.Fst.union_seq([
            gn.Fst('полпервого') + self._insert_seq('0', 'H', '30', 'M'),
            gn.Fst('полвторого') + self._insert_seq('1', 'H', '30', 'M'),
            gn.Fst('полтретьего') + self._insert_seq('2', 'H', '30', 'M'),
            gn.Fst('полчетвертого') + self._insert_seq('3', 'H', '30', 'M'),
            gn.Fst('полпятого') + self._insert_seq('4', 'H', '30', 'M'),
            gn.Fst('полшестого') + self._insert_seq('5', 'H', '30', 'M'),
            gn.Fst('полседьмого') + self._insert_seq('6', 'H', '30', 'M'),
            gn.Fst('полвосьмого') + self._insert_seq('7', 'H', '30', 'M'),
            gn.Fst('полдевятого') + self._insert_seq('8', 'H', '30', 'M'),
            gn.Fst('полдесятого') + self._insert_seq('9', 'H', '30', 'M'),
            gn.Fst('пол-одиннадцатого') + self._insert_seq('10', 'H', '30', 'M'),
            gn.Fst('полдвенадцатого') + self._insert_seq('11', 'H', '30', 'M')
        ])

    def and_some_part(self, what, whole=60):
        return gn.Fst.union_seq([
            gn.Fst.union_seq([self.w + fstring('с половиной'), ',5']) + self._insert_seq(unicode(whole / 2), what),
            self.w + fstring('с четвертью') + self._insert_seq(unicode(whole / 4), what)
        ])

    def _hours_and_some_part(self, insert_suffix=''):
        hours = self._catch(self.num_hours_24(include_24=False)) + self._insert('H%s' % insert_suffix)
        minutes = self.and_some_part(what='M%s' % insert_suffix)
        return gn.Fst.union_seq([
            # 8 с половиной часов
            hours + minutes + self.w + self.hours_noun(),
            # 8 часов с половиной
            hours + self.w + self.hours_noun() + minutes
        ])

    def _minutes_and_some_part(self, insert_suffix=''):
        minutes = self._catch(self.num_minutes()) + self._insert('M%s' % insert_suffix)
        seconds = self.and_some_part(what='S%s' % insert_suffix)
        return gn.Fst.union_seq([
            # 8 с половиной минут
            minutes + seconds + self.w + self.minutes_noun(),
            # 8 минут с половиной
            minutes + self.w + self.minutes_noun() + seconds
        ])

    def _hours_and_minutes(self):
        zero = gn.qq(self.w + '0')
        hours_and = self._hours() + gn.qq(self.w + 'и') + zero + self.w
        hour_and = self.hours_noun() + self._insert_seq('1', 'H') + zero + self.w
        wordless_minutes = self._catch(self.num_minutes()) + self._insert('M')
        # 8 40
        hh_mm = (self._catch(self.num_hours_24(False)) + self._insert('H') + zero + self.w +
                 wordless_minutes)
        # 8 часов 40 минут
        hours_mins = hours_and + self._minutes()
        # 8 часов 40
        hours_mins_wordless = hours_and + wordless_minutes
        # час 40 минут
        hour_mins = hour_and + self._minutes()
        # час 40
        hour_mins_wordless = hour_and + wordless_minutes
        return hh_mm | hours_mins | hours_mins_wordless | hour_mins | hour_mins_wordless

    def _minutes_and_seconds(self):
        mins_and = self._minutes() + gn.qq(self.w + 'и') + gn.qq(self.w + '0') + self.w
        # 8 минут 40 секунд
        mins_seconds = mins_and + self._seconds()
        # 8 минут 40
        mins_seconds_wordless = mins_and + self._catch(self.num_seconds()) + self._insert('S')
        return mins_seconds | mins_seconds_wordless

    def _hours_minutes_seconds(self):
        sep = gn.qq(self.w + 'и') + self.w
        return self._hours() + sep + self._minutes() + sep + self._seconds()

    def _time_of_day_in_middle(self):
        hours = self.hours_noun(False) + self._insert_seq('1', 'H')
        hours |= self._catch(self.num_hours_12(include_zero=False)) + self._insert('H') + gn.qq(self.w + self.hours_noun())
        time_of_day = self.w + gn.Fst.union_seq([
            'утра' + self._insert('morning'),
            'вечера' + self._insert('evening'),
            'дня' + self._insert('day'),
            'ночи' + self._insert('night')
        ])
        minutes = self.w + self._minutes()
        minutes_wordless = self.w + self._catch(self.num_minutes()) + self._insert('M')
        seconds = self.w + self._seconds()
        # 8 утра 30 минут
        hh_mm = hours + time_of_day + minutes
        # 8 утра 30
        hh_mm_wordless = hours + time_of_day + minutes_wordless
        # 8 утра 30 минут 15 секунд
        hh_mm_ss = hours + time_of_day + minutes + seconds
        return hh_mm | hh_mm_wordless | hh_mm_ss

    def _noon_midnight(self):
        return gn.Fst.union_seq([
            fst_cases('полдень') + self._insert_seq('12', 'H', '0', 'M') + self._insert('pm'),
            fst_cases('полночь') + self._insert_seq('12', 'H', '0', 'M') + self._insert('am')
        ])

    def _time_of_day(self):
        return gn.Fst.union_seq([
            fst_cases('утро') + self._insert('morning'),
            fst_cases('день') + self._insert('day'),
            fst_cases('вечер') + self._insert('evening'),
            fst_cases('ночь') + self._insert('night'),
            fstring('после полуночи') + self._insert('am'),
            (fstring('после полудня') | fstring('пополудни')) + self._insert('pm'),
            (gn.Fst('am') | 'ам' | fstring('а эм')) + self._insert('am'),
            (gn.Fst('pm') | 'пм' | fstring('пи эм') | fstring('пэ эм')) + self._insert('pm')
        ])

    def _loop(self, units):
        conj = gn.Fst('и')
        return units + gn.ss(self.w + gn.qq(conj + self.w) + units)

    def _relative_forward(self):
        next_adj = gn.Fst.union_seq(map(fst_cases, [
            'следующий', 'следующая', 'следующее', 'через'
        ]))
        units = self._loop(self._hours('+') | self._minutes('+') | self._seconds('+'))
        return next_adj + self.w + units

    def _relative_backward(self):
        past_adj = gn.Fst.union_seq(map(fst_cases, [
            'прошлый', 'прошлая', 'прошлое',
            'прошедший', 'прошедшая', 'прошедшее'
        ]))
        units = self._loop(self._hours('-') | self._minutes('-') | self._seconds('-'))
        return gn.Fst.union_seq([
            past_adj + self.w + units,
            units + self.w + 'назад'
        ])
