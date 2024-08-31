# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.common.sample import Sample

from vins_core.nlu.sample_processors.base import BaseSampleProcessor


logger = logging.getLogger(__name__)


def _drop_items(items, l):
    return [x for i, x in enumerate(l) if i not in items]


class DropCensoredSamplesProcessor(BaseSampleProcessor):
    """Sample processor for dropping <censored>
    """

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, session, *args, **kwargs):
        drop_list = []
        for i, v in enumerate(sample.tokens):
            if v == '<censored>':
                drop_list.append(i)
        if drop_list:
            updated_tokens = _drop_items(drop_list, sample.tokens)
            updated_text = ' '.join(updated_tokens).strip()
            return Sample(
                utterance=sample.utterance,
                tokens=updated_tokens,
                tags=_drop_items(drop_list, sample.tags),
                app_id=sample.app_id,
                partially_normalized_text=updated_text
            )
        return sample
