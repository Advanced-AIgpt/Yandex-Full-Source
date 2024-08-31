# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os
import re
import codecs
import json

import fasteners.process_lock as locks
from collections import defaultdict

from vins_core.ner.fst_base import V_INSERTED, V_END, V_BEG, HIERARCHY, TAG, TOK
import vins_tools.nlu.ner.normbase as gn


WSET = u" \t\r\n\u00a0"


def default_normalization(text):
    return re.sub(r'\s+', ' ', text).strip()


class NluFstBaseConstructor(object):
    def __init__(self, fst_name, normalizer=default_normalization):
        self.fst_name = fst_name

        self._fst = None

        self.normalizer = normalizer

        self.maps = {}
        self.weights = defaultdict(dict)

    def create(self):
        gn.categories_defined()
        self.w = gn.pp(' ')
        fsymb = gn.Fst.union_seq(set(gn.alphabet) - set(WSET))

        self.ftok = gn.insert(TOK)
        self.ftag = gn.insert(TAG)
        self.fhierarchy = gn.insert(HIERARCHY)
        self.ftype = gn.insert(self.fst_name.upper())
        self.filler = self.ftag + gn.pp(gn.cost(fsymb, 1000.))

        self.fsts = []

    def compile(self):
        self.create()
        if not self.fsts:
            raise ValueError('FSTs "%s" are empty after calling create()' % self.fst_name)
        self._fst = self._pipeline().optimize()

        return self

    def _save(self, fst, name, dir_):
        lock_name = os.path.join(dir_, name + '.fst-save.lock')

        try:
            with locks.InterProcessLock(lock_name):
                with gn.FSTSequenceSaver(dir_) as f:
                    f.add(fst, name)

                maps_file = os.path.join(dir_, 'maps.json')
                with codecs.open(maps_file, 'w') as fout:
                    json.dump(self.maps, fout)

                weights_file = os.path.join(dir_, 'weights.json')
                with codecs.open(weights_file, 'w') as fout:
                    json.dump(self.weights, fout)
        finally:
            try:
                os.remove(lock_name)
            except OSError:
                pass

    def save(self, path=None):
        self._save(self._fst, self.fst_name, path)

    def save_to_archive(self, archive):
        with archive.nested(self.fst_name) as arch:
            tmpdir = arch.get_tmp_dir()
            self.save(tmpdir)
            files = os.listdir(tmpdir)
            arch.add_files([os.path.join(tmpdir, fname) for fname in files])

    def _ftarget_fst(self):
        return gn.Fst.union_seq([
            self.ftype + self.fhierarchy +
            gn.cost(f, cost + 1.)
            for cost, f in enumerate(reversed(self.fsts))
        ])

    def _pipeline(self):
        return gn.ss(
            self.w + self.ftok + (self._ftarget_fst() | self.filler) + self.w
        )


class NluFstBaseValueConstructor(NluFstBaseConstructor):
    def create(self):
        super(NluFstBaseValueConstructor, self).create()

        self.vb = gn.insert(V_BEG)
        self.ve = gn.insert(V_END)
        self.vinserted = gn.insert(V_INSERTED)

    def _insert(self, what):
        return self.vb + self.vinserted + gn.insert(what) + self.ve

    def _insert_seq(self, *what_seq):
        f = gn.empty()
        for what in what_seq:
            f += self._insert(what)
        return f

    def _catch(self, what):
        return self.vb + what + self.ve
