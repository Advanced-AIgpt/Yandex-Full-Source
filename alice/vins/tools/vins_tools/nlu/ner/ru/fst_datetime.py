# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from collections import OrderedDict

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.fst_utils import (
    fst_cases_num, fst_cases, fstring, fcopy, fallperm, fst_alt, put_cases, num2text, ordinal_suffix,
    put_genders_cases)
from vins_tools.nlu.ner.fst_base import NluFstBaseValueConstructor


MONTHS = OrderedDict([
    ('январь', ('01', 1, 32)),
    ('февраль', ('02', 1, 30)),
    ('март', ('03', 1, 32)),
    ('апрель', ('04', 1, 31)),
    ('май', ('05', 1, 32)),
    ('июнь', ('06', 1, 31)),
    ('июль', ('07', 1, 32)),
    ('август', ('08', 1, 32)),
    ('сентябрь', ('09', 1, 31)),
    ('октябрь', ('10', 1, 32)),
    ('ноябрь', ('11', 1, 31)),
    ('декабрь', ('12', 1, 32))
])


class NluFstDatetimeRuConstructor(NluFstBaseValueConstructor):
    def _unit_names(self):

        fout = OrderedDict([
            ('S', fst_cases_num('секунда').optimize()),
            ('M', fst_cases_num('минута').optimize()),
            ('H', fst_cases_num('час').optimize()),
            ('d', (fst_cases_num('день') |
                   fst_cases_num('сутки')
                   ).optimize()),
            ('w', fst_cases_num('неделя').optimize()),
            ('m', fst_cases_num('месяц').optimize()),
            ('Y', (fst_cases_num('год') |
                   fst_cases_num('лет')
                   ).optimize())
        ])

        fout1 = OrderedDict([
            ('S', fst_cases('секунда').optimize()),
            ('M', fst_cases('минута').optimize()),
            ('H', fst_cases('час').optimize()),
            ('d', (fst_cases('день') |
                   fst_cases('сутки')
                   ).optimize()),
            ('w', fst_cases('неделя').optimize()),
            ('m', fst_cases('месяц').optimize()),
            ('Y', (fst_cases('год') |
                   fst_cases('лет')
                   ).optimize())
        ])
        return fout, fout1

    def _unit_logic(self):

        return {
            'S': ((), ('M', 1./60)),
            'M': (('S', 60), ('H', 1./60)),
            'H': (('M', 60), ('d', 1./24)),
            'd': (('H', 24), ('w', 1./7)),
            'w': (('d', 7), ('m', 1./4)),
            'm': (('w', 4), ('Y', 1./12)),
            'Y': (('m', 12), ())
        }

    def create(self):

        super(NluFstDatetimeRuConstructor, self).create()

        self._next_adj = gn.Fst.union_seq(map(fst_cases, [
            'следующий', 'следующая', 'следующее',
            'будущий', 'будущая', 'будущее',
            'грядущий', 'грядущая', 'грядущее'
        ]))
        self._past_adj = gn.Fst.union_seq(map(fst_cases, [
            'прошлый', 'прошлая', 'прошлое',
            'прошедший', 'прошедшая', 'прошедшее'

        ]))

        self.weekdays = [
            fst_cases('понедельник') + self.vb + gn.insert('1 W') + self.ve,
            fst_cases('вторник') + self.vb + gn.insert('2 W') + self.ve,
            fst_cases('среда') + self.vb + gn.insert('3 W') + self.ve,
            fst_cases('четверг') + self.vb + gn.insert('4 W') + self.ve,
            fst_cases('пятница') + self.vb + gn.insert('5 W') + self.ve,
            fst_cases('суббота') + self.vb + gn.insert('6 W') + self.ve,
            fst_cases('воскресенье') + self.vb + gn.insert('7 W') + self.ve,
        ]

        self.fdM = gn.g.digit | (gn.anyof('012345') + gn.g.digit)
        self.fdH = gn.g.digit | (gn.Fst('1') + gn.g.digit) | (gn.Fst('2') + gn.anyof('01234'))
        self.fdS = self.fdM
        self.fdate_suf = gn.qq(ordinal_suffix())
        self.fYd = (
            self.vb + gn.anyof('12') + gn.rr(gn.g.digit, 3, 3) + self.ve + self.fdate_suf +
            self.vb + gn.insert(' Y') + self.ve
        )
        self.fYd_years = (
            self.vb + gn.qq(gn.anyof('12')) + gn.rr(gn.g.digit, 1, 3) + self.ve + self.fdate_suf +
            self.vb + gn.insert(' Y') + self.ve +
            self.w + fst_cases('год')
        )
        self.fdate_sep = gn.anyof('./')
        self.available_days = lambda m, with_zero: num2text(range(*MONTHS[m][1:]), with_zero)
        self.funit_names, self.funit_names_single = self._unit_names()
        self.fprep = self._prep()
        self.fconj = self._conj()
        self.funits = self.units()
        self.funits_plus = gn.Fst.union_seq(self.units('+').values())
        self.funits_minus = gn.Fst.union_seq(self.units('-').values())

        f_wprepw = self.w + gn.qq(self.fprep + self.w)

        f_spec_day = self.spec_day()
        f_weekday = self.weekday()
        f_spec_day |= f_weekday
        f_timeofday_oc = self.timeofday_optional_context()
        f_timeofday_mc = self.timeofday_mandatory_context()

        f_hhmm = self.hhmm()
        f_forward = self.forward([
            self.funits_plus,
        ])
        f_backward = self.backward([
            self.funits_minus,
        ])
        f_hours_mins, f_hours_nomins = self.hours_mins()
        f_hours_mins |= f_hours_nomins
        f_hours_mins_short, f_hours_nomins_short = self.hours_mins(say_hours_mins=False)
        f_half_past = self.half_past()
        f_minute_to = self.minute_to()
        f_minute_to_short = self.minute_to(say_minute=False)
        f_minute_past = self.minute_past()
        f_noon_midnight = self.noon_midnight()

        f_ddmmYY = self.ddmmYY()
        f_day_month, f_year = self.day_month_year(decoupled=True)
        self._date = gn.Fst.union_seq([
            f_ddmmYY,
            f_day_month + gn.qq(self.w + f_year),
            self.days_suffix(),
            self.fYd_years,
            self.fYd
        ])

        f_relative = gn.Fst.union_seq([
            f_forward,
            f_backward,
            f_spec_day
        ])

        # central time expression with optional context
        f_time_coc = gn.Fst.union_seq([
            # 8:30
            f_hhmm,
            # полдевятого
            f_half_past,
            # 8 часов 30 минут
            f_hours_mins,
            # без 30 минут 9
            f_minute_to
        ])

        # central time expression with mandatory context
        f_time_cmc = gn.Fst.union_seq([
            # без 30 9
            f_minute_to_short,
            # 8
            f_hours_nomins_short,
            # 30 минут 9-го
            # TODO: move to the time with optional context when ordinals will be supported by normalizer
            f_minute_past,
            f_hours_mins_short
        ])

        f_time_oc = fallperm([f_spec_day, f_timeofday_oc], fsep=f_wprepw).optimize()
        f_time_mc = fallperm([f_spec_day, f_timeofday_mc], fsep=f_wprepw).optimize()
        f_time_loc = f_time_oc + self.w + self.fprep + self.w
        f_time_roc = f_wprepw + f_time_oc
        f_time_lmc = f_time_mc + self.w + self.fprep + self.w
        f_time_rmc = f_wprepw + f_time_mc
        f_time = gn.Fst.union_seq([

            # завтра в 8 часов утра, 8 часов утра, 8 часов
            gn.qq(f_time_loc) + f_time_coc + gn.qq(f_time_roc),

            # завтра в 8, 8 утра
            fst_alt(f_time_lmc, f_time_cmc, f_time_rmc) |
            # завтра в 8 утра
            (f_time_lmc + f_time_cmc + f_time_rmc),

            fst_alt(gn.qq(f_spec_day + f_wprepw),
                    f_noon_midnight,
                    gn.qq(self.w + f_spec_day))
        ])
        self.fsts = [
            self.ftag + f_relative,
            self.ftag + f_time,
            self.ftag + self._date,
            self.ftag + f_time + f_wprepw + self._date,
            self.ftag + self._date + f_wprepw + f_time,
        ]

    def units(self, sign='', filter_units=None):
        fu = {}
        unit_logic = self._unit_logic()
        fd = self.vb + gn.pp(gn.g.digit) + self.ve
        if filter_units:
            unit_names = filter(lambda name: name in filter_units, self.funit_names.keys())
        else:
            unit_names = self.funit_names.keys()
        for u_fmt in unit_names:
            f_fmt = self.vb + gn.insert(' %s%s' % (u_fmt, sign)) + self.ve
            f = [
                self.funit_names_single[u_fmt] + self.vb + gn.insert('1 %s%s' % (u_fmt, sign)) + self.ve,
                fd + self.w + self.funit_names[u_fmt] + f_fmt
            ]
            if unit_logic[u_fmt][0]:
                half_unit_fmt, half_unit_val = unit_logic[u_fmt][0]
                half_unit_val /= 2
                # 8 c половиной часов
                f.append(
                    fd + f_fmt + gn.Fst.union_seq([
                        self.w + fstring('с половиной'),
                        ',5'
                    ]) + self.w +
                    self.vb + gn.insert('%d %s%s' % (half_unit_val, half_unit_fmt, sign)) + self.ve +
                    self.funit_names[u_fmt]
                )
                # полчаса
                f.append(
                    'пол' + gn.qq(self.w) + self.funit_names_single[u_fmt] +
                    self.vb + gn.insert('%d %s%s' % (half_unit_val, half_unit_fmt, sign)) + self.ve
                )
                # полтора часа
                f.append(
                    'полтор' + gn.anyof('аы') + self.w + self.funit_names_single[u_fmt] +
                    self.vb + gn.insert('1 %s%s' % (u_fmt, sign)) + self.ve +
                    self.vb + gn.insert('%d %s%s' % (half_unit_val, half_unit_fmt, sign)) + self.ve
                )
                # пара часов
                f.append(fst_cases('пара') + self.w + self.funit_names[u_fmt] +
                         self.vb + gn.insert('2 %s%s' % (u_fmt, sign)) + self.ve)
            fu[u_fmt] = gn.Fst.union_seq(f)
        return fu

    def _prep(self):
        fp = gn.Fst.union_seq([
            'приблизительно',
            'примерно'
        ])
        return gn.qq(fp + self.w) + gn.anyof([
            'в', 'на', 'к', 'до'
            'перед', 'после', 'около'
        ])

    def _conj(self):
        return gn.Fst.union_seq([u'и'])

    def weekday(self):
        weekdays = gn.Fst.union_seq(self.weekdays)
        spec11 = fstring('на прошлой неделе') + self.vb + gn.insert('1 w-') + self.ve
        spec12 = fstring('на следующей неделе') + self.vb + gn.insert('1 w+') + self.ve
        past_week = self.vb + gn.insert('1 w-') + self.ve
        next_week = self.vb + gn.insert('1 w+') + self.ve
        curr_adj = gn.Fst.union_seq(map(fst_cases, [
            'ближайший', 'ближайшая', 'ближайшее'
        ]))
        spec21 = self._past_adj + past_week
        spec22 = self._next_adj + next_week
        spec3 = curr_adj
        return (
            fallperm([spec11 | spec12, weekdays], self.w) |
            gn.qq((spec21 | spec22 | spec3) + self.w) + weekdays
        )

    def spec_day(self):
        now_synonims = [
            fstring('сейчас'), fstring('в настоящий момент'), fstring('в настоящее время'),
            fstring('в текущее время'), fstring('в текущий час'), fstring('в этот час'),
            fstring('в данное время'), fstring('на данный момент')
        ]
        return gn.Fst.union_seq([
            gn.Fst.union_seq(
                ['позавчера'] + map(fstring, put_cases('позавчерашний день'))
                + map(fstring, put_genders_cases('позавчерашний'))
            ) + self.vb + gn.insert('2 d-') + self.ve,
            gn.Fst.union_seq(
                ['вчера'] + map(fstring, put_cases('вчерашний день')) + map(fstring, put_genders_cases('вчерашний'))
            ) + self.vb + gn.insert('1 d-') + self.ve,
            gn.Fst.union_seq(
                ['сегодня', fstring('течение дня')] + map(fstring, put_cases('сегодняшний день'))
                + map(fstring, put_genders_cases('сегодняшний'))
            ) + self.vb + gn.insert('0 d+') + self.ve,
            gn.Fst.union_seq(now_synonims) + self.vb + gn.insert('0 S+') + self.ve,
            gn.Fst.union_seq(
                ['завтра'] + map(fstring, put_cases('завтрашний день')) + map(fstring, put_genders_cases('завтрашний'))
            ) + self.vb + gn.insert('1 d+') + self.ve,
            gn.Fst.union_seq(
                ['послезавтра'] + map(fstring, put_cases('послезавтрашний день'))
                + map(fstring, put_genders_cases('послезавтрашний'))
            ) + self.vb + gn.insert('2 d+') + self.ve,
        ])

    def timeofday_optional_context(self):

        return gn.Fst.union_seq([
            fst_cases('утро'),
            fst_cases('день') + self.vb + gn.insert('12 H+') + self.ve,
            fst_cases('вечер') + self.vb + gn.insert('12 H+') + self.ve,
            fst_cases('ночь'),
            fstring('после полудня') + self.vb + gn.insert('12 H+') + self.ve
        ])

    def timeofday_mandatory_context(self):
        return gn.Fst.union_seq([
            fst_cases('утро'),
            fst_cases('вечер') + self.vb + gn.insert('12 H+') + self.ve,
            fst_cases('ночь'),
            fstring('после полудня') + self.vb + gn.insert('12 H+') + self.ve
        ])

    def _loop_units(self, units):
        f = units + gn.ss(
            self.w + gn.qq(self.fconj + self.w) + units
        )
        return f

    def backward(self, what):
        f = self._loop_units(
            gn.Fst.union_seq(what)
        )
        prefix_f = gn.Fst.union_seq([self._past_adj]) + self.w + f
        f_suffix = f + self.w + gn.Fst.union_seq(['назад'])
        return prefix_f | f_suffix

    def forward(self, what):
        f = self._loop_units(
            gn.Fst.union_seq(what)
        )
        prefix_f = gn.Fst.union_seq(['через', self._next_adj]) + self.w + f
        return prefix_f

    def half_past(self):
        f = [
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('первого') + self.vb + gn.insert('0 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('второго') + self.vb + gn.insert('1 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('третьего') + self.vb + gn.insert('2 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('четвертого') + self.vb + gn.insert('3 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('пятого') + self.vb + gn.insert('4 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('шестого') + self.vb + gn.insert('5 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('седьмого') + self.vb + gn.insert('6 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('восьмого') + self.vb + gn.insert('7 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('девятого') + self.vb + gn.insert('8 H 30 M') + self.ve,
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('десятого') + self.vb + gn.insert('9 H 30 M') + self.ve,
            (
                gn.Fst('пол') + (gn.ss(self.w) | gn.Fst('-')) + gn.Fst('одиннадцатого') +
                self.vb + gn.insert('10 H,30 M') + self.ve
            ),
            gn.Fst('пол') + gn.ss(self.w) + gn.Fst('двенадцатого') + self.vb + gn.insert('11 H,30 M') + self.ve,
        ]
        return gn.Fst.union_seq(f)

    def minute_to(self, say_minute=True):

        fminute_to = self.vb + self.fdM + self.ve + self.vb + gn.insert(' M- 0 M') + self.ve
        if say_minute:
            fminute_to += self.w + self.funit_names['M']
        fmin = (
            fminute_to |
            gn.Fst('четверти') + self.vb + gn.insert('15 M- 0 M') + self.ve |
            gn.Fst('половины') + self.vb + gn.insert('30 M- 0 M') + self.ve
        )
        fhour = self.vb + self.fdH + self.ve + self.vb + gn.insert(' H') + self.ve + gn.qq(':00')
        f = gn.Fst('без') + self.w + fmin
        f += self.w + fhour

        return f

    def minute_past(self, say_minute=True):

        f_minute_past = self.vb + self.fdM + self.ve + self.vb + gn.insert(' M+ 0 M 1 H-') + self.ve
        if say_minute:
            f_minute_past += self.w + self.funit_names['M']

        fmin = (
            f_minute_past |
            gn.Fst('четверть') + self.vb + gn.insert('15 M+ 0 M 1 H-') + self.ve |
            gn.Fst('половина') + self.vb + gn.insert('30 M+ 0 M 1 H-') + self.ve
        )

        f = fmin + self.w + self.vb + gn.g.digit + self.ve + self.vb + gn.insert(' H') + self.ve
        # TODO: remove qq after DIALOG-1324
        f += gn.qq('-го')
        return f

    def hhmm(self):

        fH = (
            self.vb + self.fdH + self.ve |
            gn.Fst('0') + self.vb + gn.g.digit + self.ve
        ) + self.vb + gn.insert(' H') + self.ve

        fM = self.vb + self.fdM + self.ve + self.vb + gn.insert(' M') + self.ve
        return fH + ':' + fM

    def _hours(self, say_hours=True):
        say_hours = self.w + self.funit_names['H'] if say_hours else gn.qq(self.w + self.funit_names['H'])
        fhd = gn.anyof('0123456789') | (gn.Fst('1') + gn.g.digit) | (gn.Fst('2') + gn.anyof('0123'))
        return (
            self.vb + fhd + self.ve + self.vb + gn.insert(' H') + self.ve + say_hours |
            self.funit_names_single['H'] + self.vb + gn.insert('1 H') + self.ve
        )

    def _minutes(self, say_minutes=True):
        say_mins = self.w + self.funit_names['M']
        if not say_minutes:
            say_mins = gn.qq(say_mins)
        return self.vb + self.fdM + self.ve + say_mins + self.vb + gn.insert(' M') + self.ve

    def _seconds(self):
        return self.vb + self.fdS + self.ve + self.w + self.funit_names['S'] + self.vb + gn.insert(' S') + self.ve

    def hours_mins(self, say_hours_mins=True):
        say_hours = self.w + self.funit_names['H'] if say_hours_mins else gn.qq(self.w + self.funit_names['H'])
        f_hours = self._hours(say_hours=say_hours_mins)
        f_and_half = gn.Fst.union_seq([
            self.w + fstring('с половиной'), ',5'
        ]) + self.vb + gn.insert('30 M') + self.ve

        # 8 с половиной часов, 8 часов с половиной
        f_hours_30min = f_hours + (f_and_half + say_hours | say_hours + f_and_half)
        f_mins = self._minutes(say_minutes=say_hours_mins)

        return f_hours + self.w + f_mins, (
            f_hours + self.vb + gn.insert('0 M') + self.ve |
            f_hours_30min
        )

    def noon_midnight(self):

        return gn.Fst.union_seq([
            fst_cases('полдень') + self.vb + gn.insert('12 H 0 M') + self.ve,
            fst_cases('полночь') + self.vb + gn.insert('0 H 0 M') + self.ve
        ])

    def ddmmYY(self):
        f = []
        for month, (mm, day_st, day_en) in MONTHS.iteritems():
            fdd = []
            for mday in self.available_days(month, with_zero=True):
                fdd.append(gn.Fst(mday) + self.vb + gn.insert('{0} d'.format(mday)) + self.ve)
            fdd = gn.Fst.union_seq(fdd)
            f.append(fdd + self.fdate_sep + gn.Fst(mm) + self.vb + gn.insert('{0} m'.format(mm)) + self.ve)

        f = gn.Fst.union_seq(f) + self.fdate_sep + self.fYd
        return f

    def day_month_year(self, decoupled=False):
        f = []
        # -го,-му,...
        # side normalizator effects
        for month, (mm, day_st, day_en) in MONTHS.iteritems():
            fdd = fcopy(self.available_days(month, with_zero=False), self.vb, gn.insert(' d') + self.ve)
            # fdd = gn.Fst.union_seq(self.available_days(month, with_zero=False)) + gn.insert(' d:')
            # bug here: lazy match (no case aggr)
            f.append(
                gn.qq(fdd + self.fdate_suf + self.w) +
                fst_cases(month) + self.vb + gn.insert('{0} m'.format(mm)) + self.ve
            )
        f = gn.Fst.union_seq(f)
        fy = self.fYd | self.fYd_years

        if decoupled:
            return f, fy
        else:
            return f + self.w + fy

    def days_suffix(self):
        day = gn.anyof('123456789') | gn.anyof('12') + gn.g.digit | gn.Fst('31')
        day = self.vb + day + self.ve
        day += self.vb + gn.insert(' d') + self.ve
        return day + self.w + fst_cases('число')
