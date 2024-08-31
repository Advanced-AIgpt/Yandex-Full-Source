# coding: utf-8

import re

from pynorm import fstnormalizer

from nlu_service import settings


RE_DECIMAL_COMMA = re.compile('(\d),(\d)')
REVERSE_NORMALIZER = fstnormalizer.FSTNormalizer(settings.REVERSE_NORMALIZER_RESOURCE_DIR)


def replace_decimal_commas(utterance):
    return RE_DECIMAL_COMMA.sub(r'\g<1>.\g<2>', utterance)


def reverse_normalize(utterance):
    # type: (unicode) -> unicode
    return replace_decimal_commas(REVERSE_NORMALIZER.normalize(utterance.encode('utf-8'))).decode('utf-8').lower()
