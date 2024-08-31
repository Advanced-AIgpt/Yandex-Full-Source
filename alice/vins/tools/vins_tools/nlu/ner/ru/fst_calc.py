# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import vins_tools.nlu.ner.normbase as gn

from vins_tools.nlu.ner.ru.fst_num import NluFstNumRuConstructor
from vins_tools.nlu.ner.ru.fst_float import NluFstFloatRuConstructor
from vins_tools.nlu.ner.fst_utils import fst_cases, fstring


class NluFstCalcRuConstructor(NluFstFloatRuConstructor, NluFstNumRuConstructor):

    def create(self):

        super(NluFstCalcRuConstructor, self).create()

        self.fprep = gn.Fst.union_seq(['на', 'к'])
        ops = self.ops()
        self.args = self.get_args()
        single_args = self.trigonometry() | self.multiply()
        any_args = self.args | single_args
        self.fsts = [
            # cos(3.45), трижды девять
            self.ftag + single_args,
            # pi
            self.ftag + self.consts(),
            # раздели 45 (на 10)
            self.ftag + ops + self.w + any_args + gn.qq(self.w + self.fprep + self.w + any_args),
            # к 22 прибавь 16
            self.ftag + self.fprep + self.w + any_args + self.w + ops + self.w + any_args,
            # 69 умножить на arcsin(0.5)
            self.ftag + any_args + self.w + ops + self.w + any_args + gn.ss(self.w + ops + self.w + any_args),
        ]

    def get_args(self):
        return self.num() | self.float() | self.fractions() | self.consts()

    def ops(self):
        suffix = gn.qq(self.w + self.fprep)
        utt_ops = gn.Fst.union_seq([
            'плюс', 'прибавить', 'приплюсовать', 'прибавь', 'добавь', 'сложить', 'сложи',
            'минус', 'вычесть', 'отнять', 'отними', 'вычитай',
            gn.Fst.union_seq(['до', 'у']) + 'множ' + gn.Fst.union_seq(['ить', 'и', 'ь', 'ай']) + suffix,
            gn.qq(gn.Fst.union_seq(['по', 'раз'])) + 'дел' + gn.Fst.union_seq(['ить', 'и', 'яй']) + suffix
        ])
        sym_ops = gn.Fst.union_seq(['+', '-', '*', '/', 'x'])
        return utt_ops | sym_ops

    def trigonometry(self):
        prefix = gn.Fst.union_seq([
            fst_cases('гиперболический') + self.w,
            'арк'
        ])
        utts = gn.qq(prefix) + gn.Fst.union_seq(map(fst_cases, [
            'косинус', 'синус', 'тангенс', 'котангенс'
        ]))
        sym_trig = gn.Fst.union_seq([
            'cos', 'sin', 'tan', 'tg', 'atan', 'ctan', 'ctg',
            'sinh', 'cosh', 'tanh'
        ])
        arg = (
            self.w + gn.qq(gn.Fst('от') + self.w) + self.args |
            gn.anyof('[(') + self.args + gn.anyof('(]')
        )
        return (utts | sym_trig) + gn.qq(arg)

    def multiply(self):
        return gn.Fst.union_seq([
            'дважды', 'трижды', 'четырежды', 'пятью', 'шестью', 'семью', 'восемью', 'девятью'
        ]) + self.w + gn.g.digit

    def consts(self):
        return gn.Fst.union_seq([
            gn.Fst.union_seq(['пи', 'pi']) + gn.qq(self.w + gn.Fst.union_seq(['пополам', fstring('на два')])),
            fst_cases('экспонента'), 'exp'
        ])
