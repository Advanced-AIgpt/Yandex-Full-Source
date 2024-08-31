# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.common.sample import Sample

from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.utils.strings import isnumeric


class AddressFractionSamplesProcessor(BaseSampleProcessor):
    """Sample processor for concatting fractions in addresses
    """

    @property
    def is_normalizing(self):
        return True

    def _process(self, sample, session, *args, **kwargs):
        for i in range(0, len(sample.tokens) - 2):
            if isnumeric(sample.tokens[i]) and sample.tokens[i + 1] == '/' and isnumeric(sample.tokens[i + 2]):
                return Sample.from_string(
                    item=' '.join(
                        sample.tokens[:i] +
                        [sample.tokens[i] + sample.tokens[i + 1] + sample.tokens[i + 2]] +
                        sample.tokens[i + 3:]
                    )
                )
        return sample
