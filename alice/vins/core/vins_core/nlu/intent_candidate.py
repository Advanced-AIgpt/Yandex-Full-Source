# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import attr


@attr.s
class IntentCandidate(object):
    name = attr.ib()
    score = attr.ib(default=0.0)
    fallback_score = attr.ib(default=0.0)
    transition_model_score = attr.ib(default=1.0)
    has_priority_boost = attr.ib(default=False)
    is_in_fixlist = attr.ib(default=False)
    is_active_slot = attr.ib(default=False)
    is_fallback = attr.ib(default=False)
    name_for_reranker = attr.ib(default=None)
    is_fake = attr.ib(default=False)

    def __attrs_post_init__(self):
        if self.name_for_reranker is None:
            self.name_for_reranker = self.name

    def to_dict(self):
        return attr.asdict(self)
