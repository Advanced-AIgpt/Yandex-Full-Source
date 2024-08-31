# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import copy
import logging
from collections import defaultdict

logger = logging.getLogger(__name__)


class NluFstParser(object):

    def __init__(self, normalizer, parsers, post):
        self.normalizer = normalizer
        self.parsers = parsers
        self.post = post

    def __radd__(self, other):
        self.parsers += other.parsers

    def parse(self, sample, req_info=None, **kwargs):
        if not sample:
            return []

        if self.normalizer is not None:
            sample = copy.copy(sample)
            sample.text = self.normalizer(sample.text.lower())

        if not sample.text:
            return []

        parse_results = defaultdict(list)
        for parser in self.parsers:
            parse_results[parser.fst_name].extend(parser(sample))

        post_results = self.post(parse_results, sample.text, **kwargs)
        return post_results

    def extract_ners(self, parse_result):
        return [p[1] for p in parse_result[0]['ners']] if len(parse_result) > 0 else []

    def __call__(self, utt, *args, **kwargs):
        return self.extract_ners(self.parse(utt, **kwargs))
