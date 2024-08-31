# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from alice.nlu.py_libs.utils.sample import Sample as SimpleSample
from alice.nlu.py_libs.utils.sample_normalizer import SampleNormalizer, FstNormalizerError  # noqa
from vins_core.common.sample import Sample
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.ner.fst_normalizer import normalizer_factory


class NormalizerEmptyResultError(ValueError):
    pass


class NormalizeSampleProcessor(BaseSampleProcessor):
    def __init__(self, normalizer):
        super(NormalizeSampleProcessor, self).__init__()
        if not normalizer:
            raise ValueError('Normalizer is not specified')
        self._sample_normalizer = SampleNormalizer(normalizer_factory.get_normalizer(normalizer))

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, *args, **kwargs):
        text = sample.text
        utterance = sample.utterance
        weight = sample.weight
        app_id = sample.app_id
        annotations = sample.annotations
        partially_normalized_text = sample.partially_normalized_text
        sample = self._sample_normalizer.normalize(SimpleSample(tokens=sample.tokens, tags=sample.tags))
        if len(sample) == 0:
            raise NormalizerEmptyResultError('Normalizer returns empty result for input "%s"' % text)
        return Sample(
            utterance=utterance,
            tokens=sample.tokens,
            tags=sample.tags,
            annotations=annotations,
            weight=weight,
            app_id=app_id,
            partially_normalized_text=partially_normalized_text
        )
