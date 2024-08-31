# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.common.sample import Sample
from vins_core.nlu.sample_processors.base import BaseSampleProcessor


logger = logging.getLogger(__name__)


class ClipSamplesProcessor(BaseSampleProcessor):
    """
    Clip input utterance to specified number of tokens
    """
    def __init__(self, max_tokens=256, **kwargs):
        assert max_tokens > 0, 'Maximum number of tokens should be greater than zero'
        super(ClipSamplesProcessor, self).__init__(**kwargs)
        self._max_tokens = max_tokens

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, session, *args, **kwargs):
        updated_tokens = sample.tokens[:self._max_tokens]
        updated_text = ' '.join(updated_tokens).strip()
        return Sample(
            utterance=sample.utterance,
            tokens=updated_tokens,
            tags=sample.tags[:self._max_tokens],
            app_id=sample.app_id,
            partially_normalized_text=updated_text,
            annotations=sample.annotations,
        )
