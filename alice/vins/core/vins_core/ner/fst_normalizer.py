# coding: utf-8

from __future__ import unicode_literals

import os
import re
import logging

from pynorm import fstnormalizer

from vins_core.utils.data import get_resource_full_path
from vins_core.utils.strings import utf8call


FST_BLACKLIST = [
    'reverse_conversion.profanity',
    'reverse_conversion.make_substitution_group',
    'number.convert_size',
    'units.converter',
    'reverse_conversion.times',
]

logger = logging.getLogger(__name__)


class NluFstNormalizer(object):
    UNKNOWN_SYMBOLS_REGEXP = re.compile(
        '[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
        'абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ'
        'äáàâãāçëéèêẽēğıïíìîĩīöóòôõōşüúùûũūßœæÿñ'
        'ÄÁÀÂÃĀÇËÉÈÊẼĒĞİÏÍÌÎĨĪÖÓÒÔÕŌŞÜÚÙÛŨŪŒÆŸÑ'
        'ґієїҐІЄЇ¿¡'
        ' \t\r\n\u00a0'
        r'!"#%&\'()*+,-./0123456789:;<=>?@\[\]\\\^_`{|}~’‘´ ́ ̀'
        '±×€™§©®$‰№«»•’“”…₽£¢₴₺£₤¥‐‑‒–—―'
        # Symbols that can occur in combinations that look like Russian letters.
        '\u0302\u0306\u0308\u0327]')

    def __init__(self, normalizer=None, denormalizer=None):
        self.normalizer = normalizer
        self.denormalizer = denormalizer

    @staticmethod
    def _load_normalizer_from_resource(resource, fst_black_list=()):
        path = get_resource_full_path(resource)
        model_name = os.listdir(path)[0]
        return fstnormalizer.FSTNormalizer(os.path.join(path, model_name), fst_black_list=fst_black_list)

    @classmethod
    def from_resource(cls, normalizer_resource, denormalizer_resource, fst_black_list=()):
        normalizer = cls._load_normalizer_from_resource(normalizer_resource, fst_black_list)
        denormalizer = cls._load_normalizer_from_resource(denormalizer_resource, fst_black_list)
        return cls(normalizer=normalizer, denormalizer=denormalizer)

    def normalize(self, s):
        # special normalizer fix:
        # unable to denormalize composite tokens (digit+word)
        # check test_currency_ru_1
        out = []
        for w in s.split():
            out.append(
                utf8call(self.normalizer.normalize, w) if w.isdigit()
                else w
            )
        return ' '.join(out)

    @classmethod
    def has_unknown_symbol(cls, s):
        return cls.UNKNOWN_SYMBOLS_REGEXP.search(s) is not None

    def __call__(self, utt, *args, **kwargs):
        out = utf8call(
            self.denormalizer.normalize,
            self.normalize(utt)
        ).lower()
        return out


class NormalizerFactory(object):
    def __init__(self, config):
        self._config = config
        self._normalizers = {}

    def _load_normalizer(self, name):
        if name in self._normalizers:
            raise ValueError('Trying to load already loaded normalizer with name %s' % name)
        if name not in self._config:
            raise ValueError('No normalizer defined for name %s' % name)
        normalizer_cfg = self._config[name].get('normalizer', {})
        denormalizer_cfg = self._config[name].get('denormalizer', {})
        normalizer = NluFstNormalizer.from_resource(
            normalizer_cfg.get('resource'),
            denormalizer_cfg.get('resource'),
            fst_black_list=FST_BLACKLIST
        )
        self._normalizers[name] = normalizer
        return normalizer

    def get_normalizer(self, name):
        normalizer = self._normalizers.get(name)
        if normalizer:
            return normalizer
        return self._load_normalizer(name)


# NB: Normalizer is singleton still, because there is no simple way to include it in pipeline directly.
# There are several places that use normalizer directly: filters.number_to_word, VideoGalleryItemSelector
# and NormalizeSampleProcessor. That's why normalizer is loaded at the first address to it and that's
# why config is placed here.
# TODO: Try to include normalizer in the pipeline directly

DEFAULT_RU_NORMALIZER_NAME = 'normalizer_ru'
NORMALIZER_CONFIG = {
    "normalizer_ru": {
        "normalizer": {
            "resource": "resource://normalizer"
        },
        "denormalizer": {
            "resource": "resource://denormalizer"
        }
    }
}

normalizer_factory = NormalizerFactory(NORMALIZER_CONFIG)
