# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import os
import re
import ujson

from pynorm import fstnormalizer
from collections import defaultdict

from vins_core.common.entity import Entity
from vins_core.ner.fst_normalizer import FST_BLACKLIST
from vins_core.ner.ner_mixin import NluNerMixin
from vins_core.utils.strings import utf8call, isnumeric

logger = logging.getLogger(__name__)

TOK = '|'
TAG = '#'
HIERARCHY = '.'
V_BEG = '{'
V_END = '}'
V_INSERTED = '!'


def tokenize(s):
    return s.split()


class NluFstBase(NluNerMixin):
    def __init__(self, fst_name, fst_dir=None, fst_path=None, **kwargs):
        self.fst_name = fst_name
        self.label = self.fst_name

        if fst_path:
            self._fst_path = fst_path
        else:
            if fst_dir is None:
                self._fst_path = None
            else:
                self._fst_path = os.path.join(fst_dir, fst_name)

        if self._fst_path and os.path.exists(self._fst_path):
            self._decoder = fstnormalizer.FSTNormalizer(self._fst_path, fst_black_list=FST_BLACKLIST)
        else:
            self._decoder = None

        self.maps = self._load_maps_json(self._fst_path)
        self.weights = self._load_weights_json(self._fst_path)

    def _load_maps_json(self, path):
        if path is None:
            return {}

        maps_file = os.path.join(path, 'maps.json')
        if os.path.exists(maps_file):
            return ujson.loads(open(maps_file, 'r').read())
        else:
            return {}

    def _load_weights_json(self, path):
        if path is None:
            return defaultdict(dict)

        weights_file = os.path.join(path, 'weights.json')
        if os.path.exists(weights_file):
            return ujson.loads(open(weights_file, 'r').read())
        else:
            return defaultdict(dict)

    def _decode(self, utt):
        if not self._decoder:
            raise ValueError("Decoder '%s' not loaded." % self.fst_name)
        input_str = ' ' + re.sub(r'\s+', '  ', utt) + ' '
        return utf8call(self._decoder.normalize, input_str)

    def parse_value(self, type, value):
        value = value.strip()
        substr = value
        if isnumeric(value):
            value = int(value)  # may be more general converter needed
        return value, substr, None

    def parse_token(self, token):
        type, value = token.split(TAG)
        value = self.replace_special_symbols(value, back=True)
        type = type.rstrip(HIERARCHY)
        value, substr, weight = self.parse_value(type, value)
        value = self._process_value(value)
        substr = re.sub(r'\s+', ' ', substr.strip())
        return type, value, substr, weight

    def _process_value(self, value):
        return value

    @staticmethod
    def replace_special_symbols(utt, back=False):
        if back:
            utt = utt.replace(
                '<<SPECIAL_TAG_SYMBOL>>', TAG
            ).replace(
                '<<SPECIAL_TOK_SYMBOL>>', TOK
            )
        else:
            utt = utt.replace(
                TAG, '<<SPECIAL_TAG_SYMBOL>>'
            ).replace(
                TOK, '<<SPECIAL_TOK_SYMBOL>>'
            )
        return utt

    def parse(self, sample):
        ntoks = 0
        out = []
        utt = self.replace_special_symbols(sample.text)
        p_utt = self._decode(utt)
        tokens = p_utt.split(TOK)[1:]
        for tok in tokens:
            try:
                type, value, substr, weight = self.parse_token(tok)
            except Exception:
                logger.error('parse_token failed: %s', p_utt, exc_info=True)
                raise
            n = len(tokenize(substr))
            info = Entity(
                start=ntoks,
                end=ntoks + n,
                type=type,
                value=value,
                substr=substr,
                weight=weight
            )
            out.append(info)
            ntoks += n

        return out


class NluFstBaseValue(NluFstBase):
    def parse_value(self, type, value):

        if V_BEG not in value:
            return super(NluFstBaseValue, self).parse_value(type, value)
        else:
            substr = value.strip()
            value = re.findall(r'\{([^\}]+)\}', substr)
            for i in xrange(len(value)):
                value[i] = self._try_parse_number(value[i].lstrip(V_INSERTED))
            # remove inserted tokens
            substr = re.sub('{0}{1}[^{2}]+{2}'.format(V_BEG, V_INSERTED, V_END), '', substr)
            # remove brackets around catch tokens
            substr = re.sub('[%s%s]' % (V_BEG, V_END), '', substr)
            if len(value) == 1:
                value = value[0]
            return value, substr, None

    def _try_parse_number(self, number):
        try:
            number = int(number)
        except ValueError:
            try:
                number = float(number.replace(',', '.', 1))
            except ValueError:
                pass
        return number
