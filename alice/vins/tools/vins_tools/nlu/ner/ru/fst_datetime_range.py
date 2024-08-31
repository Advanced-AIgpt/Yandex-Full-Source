# coding: utf-8
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.ru.fst_datetime import NluFstDatetimeRuConstructor, MONTHS
from vins_tools.nlu.ner.fst_utils import fcopy, fst_cases


class NluFstDatetimeRangeRuConstructor(NluFstDatetimeRuConstructor):
    def create(self):

        super(NluFstDatetimeRangeRuConstructor, self).create()

        self.fdate_suf = gn.qq(gn.qq('-') + gn.Fst.union_seq([
            'ый', 'ой', 'ий', 'им', 'ом',
            'го', 'му', 'ым', 'ою', 'ем',
            'е', 'я', 'й', 'ю'
        ]))

        self.fstart = self.vb + gn.insert('s:') + self.ve
        self.fend = self.vb + gn.insert('e:') + self.ve

        f_from_to_date = self.from_to_date()
        f_from_to_date_no_month = self.from_to_date_no_month()
        f_from_to_monthdays = self.from_to_monthdays()
        f_next_n = self.next_n()
        f_today_tomorrow_after_tomorrow = self.today_tomorrow_after_tomorrow()
        f_on_weekend = self.on_weekend()
        f_next_weekend = self.next_weekend()
        f_on_holidays = self.on_holidays()
        f_from_to_weekday = self.from_to_weekday()
        f_next_week = self.next_week()

        self.fsts = [
            self.ftag + f_next_n,
            self.ftag + f_next_week,
            self.ftag + f_from_to_date_no_month,
            self.ftag + f_from_to_date,
            self.ftag + f_today_tomorrow_after_tomorrow,
            self.ftag + f_on_weekend,
            self.ftag + f_next_weekend,
            self.ftag + f_on_holidays,
            self.ftag + f_from_to_weekday,
            self.ftag + f_from_to_monthdays
        ]

    def from_to_monthdays(self):
        f = []
        fsep = gn.Fst.union_seq([
            '-',
            self.w,
            self.w + (gn.Fst('и') | '-') + self.w,
        ])
        for month, (mm, day_st, day_en) in MONTHS.iteritems():
            fmm = self.vb + gn.insert('{0} m'.format(mm)) + self.ve
            fmonth = fst_cases(month) + fmm
            for day_i in xrange(day_st, day_en - 1):
                fday_i = unicode(day_i) + self.vb + gn.insert('{0} d'.format(day_i)) + self.ve + fmm
                for day_j in xrange(day_i + 1, day_en):
                    fday_j = unicode(day_j) + self.vb + gn.insert('{0} d'.format(day_j)) + self.ve
                    f.append(self.fstart + fday_i + fsep + self.fend + fday_j + self.w + fmonth)
        return gn.Fst.union_seq(f)

    def from_to_date(self):
        ffrom = gn.Fst.union_seq([
            'с', 'от'
        ]) + self.w + self.fstart
        fto = gn.Fst.union_seq([
            'до', 'по'
        ]) + self.w + self.fend

        mdays = []
        for month, (mm, day_st, day_en) in MONTHS.iteritems():
            fdd = fcopy(self.available_days(month, with_zero=False), self.vb, gn.insert(' d') + self.ve)
            fmm = fst_cases(month) + self.vb + gn.insert('{0} m'.format(mm)) + self.ve
            mdays.append((fdd + self.fdate_suf + self.w, fmm, mm))

        f = []
        for i, (date_i, month_name_i, month_repr) in enumerate(mdays):
            for j, (date_j, month_name_j, _) in enumerate(mdays):
                if i < j:
                    f.append(ffrom + date_i + month_name_i + self.w + fto + date_j + month_name_j)
                elif i == j:
                    month_name_or_ommited = (
                        month_name_i + self.w |
                        self.vb + gn.insert('{0} m'.format(month_repr)) + self.ve
                    )
                    f.append(ffrom + date_i + month_name_or_ommited + fto + date_j + month_name_j)
                elif i > j:
                    f.append(
                        ffrom + date_i + month_name_i + self.w + fto + date_j + month_name_j +
                        self.vb + gn.insert('1 Y+') + self.ve
                    )
        return gn.Fst.union_seq(f)

    def from_to_date_no_month(self):
        ffrom = gn.Fst.union_seq(['с']) + self.w + self.fstart
        fto = gn.Fst.union_seq(['по']) + self.w + self.fend
        fd = gn.Fst.union_seq([gn.anyof('123456789'), gn.anyof('123') + gn.g.digit])
        fday = self.vb + gn.insert(' d') + self.ve
        fsuffix = gn.qq(self.w + 'число')
        return ffrom + self.w + self._catch(fd) + fday + self.w + fto + self.w + self._catch(fd) + fday + fsuffix

    def from_to_weekday(self):
        ffrom = gn.Fst.union_seq([
            'с', 'со'
        ]) + self.w + self.fstart
        fillers_from = gn.qq(gn.Fst.union_seq(['этого', 'этой']) + self.w)
        fto = gn.Fst.union_seq([
            'до', 'по'
        ]) + self.w + self.fend
        fillers_to = gn.qq(gn.Fst.union_seq([
            fst_cases('следующий') | fst_cases('следующая'),
            fst_cases('ближайший') | fst_cases('ближайшая')
        ]) + self.w)
        ffrom += fillers_from
        fto += fillers_to
        f = []
        for i, weekday_i in enumerate(self.weekdays):
            wi = weekday_i
            for j, weekday_j in enumerate(self.weekdays):
                wj = weekday_j
                fww = ffrom + wi + self.w + fto + wj
                if i < j:
                    f.append(fww)
                    f.append(self.fstart + self.vb + gn.insert('0 d+') + self.ve + fto + wj)
                elif i > j:
                    f.append(fww + self.vb + gn.insert('1 w+') + self.ve)
        return gn.Fst.union_seq(f)

    def next_week(self):
        next_adj = gn.Fst.union_seq([
            fst_cases('следующая'), fst_cases('та')
        ]) + self.w
        week = fst_cases('неделя')
        return (
            self.fstart + self.vb + gn.insert('1 w+') + self.ve +
            self.fend + self.vb + gn.insert('2 w+') + self.ve +
            next_adj + week
        )

    def next_n(self):
        next_following = gn.Fst.union_seq([
            fst_cases('ближайший'), fst_cases('ближайшая'), fst_cases('ближайшие'),
            fst_cases('следующий'), fst_cases('следующая'), fst_cases('следующие'),
            fst_cases('этот'), fst_cases('это'), fst_cases('эта'), fst_cases('эти')
        ]) + self.w
        return self.fend + gn.qq(next_following) + self.funits_plus

    def today_tomorrow_after_tomorrow(self):
        sep = gn.Fst.union_seq([
            '-',
            self.w + gn.qq('и' + self.w) + gn.qq(gn.Fst.union_seq(['на', 'до']) + self.w),
        ])
        today_tom = (
            self.fstart + self.vb + gn.insert('0 d+') + self.ve +
            self.fend + self.vb + gn.insert('2 d+') + self.ve +
            'сегодня' + sep + 'завтра'
        )
        today_tom_after = (
            self.fstart + self.vb + gn.insert('0 d+') + self.ve +
            self.fend + self.vb + gn.insert('3 d+') + self.ve +
            'сегодня' + sep + 'завтра' + sep + 'послезавтра'
        )
        tom_after = (
            self.fstart + self.vb + gn.insert('1 d+') + self.ve +
            self.fend + self.vb + gn.insert('3 d+') + self.ve +
            'завтра' + sep + 'послезавтра'
        )
        return today_tom | today_tom_after | tom_after

    def on_weekend(self):
        fillers = gn.qq(gn.Fst.union_seq([
            gn.Fst('ближайши') + gn.anyof('ех'),
            fst_cases('эти') + gn.qq('х')
        ]) + self.w)
        weekend = gn.Fst.union_seq([
            gn.Fst('выходны') + gn.anyof('ех'),
            gn.anyof('ув') + 'икенд' + gn.qq(gn.anyof('аеу'))
        ])
        return (
            self.fstart + self.vb + gn.insert('0 E+') + self.ve +
            self.fend + self.vb + gn.insert('0 E+') + self.ve +
            fillers + weekend
        )

    def next_weekend(self):
        next_adj = gn.Fst.union_seq([
            fst_cases('следующие'), fst_cases('следующий')
        ]) + self.w
        weekend = gn.Fst.union_seq([
            gn.Fst('выходны') + gn.anyof('ех'),
            gn.anyof('ув') + 'икенд' + gn.qq(gn.anyof('аеу'))
        ])
        return (
            self.fstart + self.vb + gn.insert('1 E+') + self.ve +
            self.fend + self.vb + gn.insert('1 E+') + self.ve +
            next_adj + weekend
        )

    def on_holidays(self):
        fillers = gn.qq(gn.Fst.union_seq([
            fst_cases('ближайшие'),
            fst_cases('эти')
        ]) + self.w)
        return (
            self.fstart + self.vb + gn.insert('0 O+') + self.ve +
            self.fend + self.vb + gn.insert('0 O+') + self.ve +
            fillers + fst_cases('праздники')
        )
