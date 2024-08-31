# coding: utf-8

from vins_core.utils.misc import dict_zip
from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor


class CharIndexerFeaturesPostProcessor(BaseFeaturesPostProcessor):

    def __init__(self, **kwargs):
        super(CharIndexerFeaturesPostProcessor, self).__init__()
        self._alphabet = list(set(
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "-,;.!?:’’’/\\|_@#$%ˆ&*˜‘+-=<>()[]{}"
            "абвгдеёжзийклмнопрстуфхцчшщъыьэюя "
        ))
        self._vectorizer = dict_zip(self._alphabet, xrange(1, len(self._alphabet) + 1))

    def transform(self, batch_features):
        output = []
        for sample_features in batch_features:
            output.append(
                [self._vectorizer.get(symbol, 0) for symbol in sample_features.sample.text.lower()]
            )
        return output
