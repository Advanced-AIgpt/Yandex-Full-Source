# coding: utf-8
from __future__ import unicode_literals

import os

import pandas as pd

import vins_tools.nlu.ner.normbase as gn

from vins_core.ner.fst_base import HIERARCHY
from vins_core.utils import data as utils_data

from vins_tools.nlu.ner.fst_custom import NluFstCustomHierarchyConstructor
from vins_tools.nlu.ner.fst_utils import put_cases


class NluFstFioRuConstructor(NluFstCustomHierarchyConstructor):
    _NAME = 'NAME'
    _SURNAME = 'SURNAME'
    _PATRONYM = 'PATRONYM'

    @classmethod
    def _load_names(cls, names_file, most_freq=None):
        names = pd.read_csv(names_file, sep='\t', header=None,
                            names=['name', 'freq0', 'freq1', 'attr'], encoding='utf-8')
        if most_freq:
            names = names.sort_values(by='freq0', ascending=False)[:most_freq]
        # sorting is important, because we want popular names to be the last in the list
        # when updating self.maps, the names from the end of the list rewrite those from the start
        names.sort_values(by='freq0', ascending=True, inplace=True)
        return names.name.tolist()

    def _load_entities(self, file_path):
        return None

    def create(self):
        super(NluFstFioRuConstructor, self).create()

        f_name = self._names(type=self._NAME, input_file='ru.first.txt')
        f_surname = self._names(type=self._SURNAME, input_file='ru.second.txt', most_freq=10000)
        f_patronym = self._patronym()
        f_initials = self._initials()

        self.fsts = [
            f_surname + self.w + f_name + gn.qq(self.w + f_patronym),
            f_name + gn.qq(self.w + f_patronym) + self.w + f_surname,
            f_surname + self.w + f_initials,
            f_initials + self.w + f_surname,
            f_name,
            f_surname
        ]

    def _names(self, type, input_file, most_freq=None):
        names = self._load_names(os.path.join(
            utils_data.find_dir('vins_tools', 'nlu/ner'),
            'data', 'fio', input_file
        ), most_freq)
        fnames = []
        t = self.fst_name.upper() + HIERARCHY + type
        self.maps[t] = {}
        for name in names:
            fname = []
            self.maps[t][name] = name
            for l_name in put_cases(name, fio=True):
                fname.append(l_name)
                self.maps[t][l_name] = name
            fnames.append(gn.Fst.union_seq(fname))
        return self.ftok + gn.insert(t) + self.ftag + gn.Fst.union_seq(fnames)

    def _patronym(self):
        fsuf = gn.anyof([
            'ович', 'овича', 'овичу', 'овичем',
            'овна', 'овны', 'овне', 'овной',
            'евич', 'евича', 'евичу', 'евичем',
            'евна', 'евны', 'евне', 'евной',
            'ич', 'ича', 'ичу', 'ичем',
            'ична', 'ичны', 'ичне', 'ичной',
            'инична', 'иничны', 'иничне', 'иничной'
        ])
        return self.ftok + gn.insert(
            '{0}.{1}'.format(self.fst_name.upper(), self._PATRONYM)
        ) + self.ftag + gn.pp(gn.g.letter) + fsuf

    def _initials(self):
        name = self.ftok + gn.insert(
            '{0}.{1}'.format(self.fst_name.upper(), self._NAME)
        ) + self.ftag + gn.g.letter
        patr = self.ftok + gn.insert(
            '{0}.{1}'.format(self.fst_name.upper(), self._PATRONYM)
        ) + self.ftag + gn.g.letter
        ipip = name + gn.remove('.') + gn.qq(self.w) + patr + gn.qq(gn.remove('.'))
        ii = name + self.w + patr
        return ipip | ii

    def _pipeline(self):
        ftarget = gn.Fst.union_seq([
            gn.cost(f, cost + 1.)
            for cost, f in enumerate(self.fsts)
        ])
        return gn.ss(
            self.w + (ftarget | self.ftok + self.filler) + self.w
        )
