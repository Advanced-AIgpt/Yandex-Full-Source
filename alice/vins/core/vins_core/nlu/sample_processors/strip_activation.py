# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.common.sample import Sample
from vins_core.common.utterance import Utterance
from vins_core.nlu.sample_processors.base import BaseSampleProcessor

logger = logging.getLogger(__name__)


class StripActivationSamplesProcessor(BaseSampleProcessor):
    """Sample processor for stripping voice activation commands form utterance
    """

    DEFAULT_FRONT_ACTIVATIONS = {
        'слушай яндекс', 'ok яндекс', 'окей яндекс', 'ok yandex', 'хэй яндекс',
        'привет яндекс', 'эй яндекс', 'ok google', 'окей гугл', 'hey yandex',
        'говорите слушай яндекс', 'говорите яндекс', 'здравствуй яндекс',
        'не слушай яндекс', 'окей google', 'окей яндекс', 'послушать яндекс',
        'привет яндекс', 'скажи яндекс', 'слушай яндекс', 'слушать яндекс',
        'слышь яндекс', 'яндекс говори', 'яндекс говорите', 'яндекс слушай',
        'яндекс слушай меня', 'яндекс слушай яндекс', 'яндекс слушать',
        'окей google', 'яндекс пожалуйста', 'пожалуйста яндекс', 'слушай яндекс пожалуйста'
    }

    DEFAULT_BACK_ACTIVATIONS = set()

    def __init__(
        self,
        apply_to_text_input=False,
        min_tokens_after_short_strip=2,
        min_tokens_after_long_strip=1,
        custom_front_activations=(),
        custom_back_activations=(),
        **kwargs
    ):
        super(StripActivationSamplesProcessor, self).__init__(**kwargs)

        self._min_tokens_after_short_strip = min_tokens_after_short_strip
        self._min_tokens_after_long_strip = min_tokens_after_long_strip
        self._apply_to_text_input = apply_to_text_input

        self._front_activations = self.DEFAULT_FRONT_ACTIVATIONS | set(custom_front_activations)
        self._front_activation_token_counts = self._activation_token_counts(self._front_activations)

        self._back_activations = self.DEFAULT_BACK_ACTIVATIONS | set(custom_back_activations)
        self._back_activation_token_counts = self._activation_token_counts(self._back_activations)

    @property
    def is_normalizing(self):
        return True

    @staticmethod
    def _activation_token_counts(activations):
        return sorted({len(phrase.split(' ')) for phrase in activations}, reverse=True)

    def _strip_activations(self, sample, activations, token_counts, from_back=False):
        for strip_len in token_counts:
            remaining_len = len(sample.tokens) - strip_len
            threshold = self._min_tokens_after_long_strip if strip_len > 1 else self._min_tokens_after_short_strip
            if remaining_len < threshold:
                continue

            stripped_tokens = sample.tokens[-strip_len:] if from_back else sample.tokens[0: strip_len]
            remaining_tokens = sample.tokens[0: -strip_len] if from_back else sample.tokens[strip_len:]
            remaining_tags = sample.tags[0: -strip_len] if from_back else sample.tags[strip_len:]

            if ' '.join(stripped_tokens) in activations:
                return Sample(
                    utterance=sample.utterance,
                    tokens=remaining_tokens,
                    tags=remaining_tags,
                    annotations=sample.annotations,
                    app_id=sample.app_id,
                    partially_normalized_text=sample.partially_normalized_text
                )

        return sample

    def _process(self, sample, session, *args, **kwargs):
        # If utterance isn't from speechkit, then there is no need to strip voice activation.
        if sample.utterance.input_source != Utterance.VOICE_INPUT_SOURCE and not self._apply_to_text_input:
            return sample

        stripped_sample = self._strip_activations(
            sample, self._front_activations, self._front_activation_token_counts, from_back=False)
        stripped_sample = self._strip_activations(
            stripped_sample, self._back_activations, self._back_activation_token_counts, from_back=True)

        return stripped_sample
