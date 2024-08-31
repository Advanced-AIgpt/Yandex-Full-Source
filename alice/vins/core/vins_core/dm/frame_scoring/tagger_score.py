# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import operator
import itertools
import numpy as np

from vins_core.dm.frame_scoring.base import FrameScoring
from vins_core.utils.misc import EPS


class TaggerScoreFrameScoring(FrameScoring):

    def __init__(self, normalize=False, multiply_confidence=False):
        self._normalize = normalize
        self._multiply_confidence = multiply_confidence

    def _call(self, frames, session, **kwargs):
        if self._multiply_confidence:
            confidences = map(operator.itemgetter('confidence'), frames)
            tagger_scores = [1.0 if x is None else x for x in map(operator.itemgetter('tagger_score'), frames)]
            scores = np.fromiter(itertools.imap(
                operator.mul,
                confidences,
                tagger_scores
            ), dtype=np.float64)
        else:
            scores = np.array(map(operator.itemgetter('tagger_score'), frames))
        if self._normalize:
            scores /= max(np.sum(scores), EPS)
        return scores
