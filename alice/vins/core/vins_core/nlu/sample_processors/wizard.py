# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import operator

from collections import defaultdict
from requests import RequestException

from vins_core.common.annotations import WizardAnnotation
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.ext.wizard_api import WizardHTTPAPI


logger = logging.getLogger(__name__)


class WizardSampleProcessor(BaseSampleProcessor):
    """
    Wizard sample processor only extracts and adds WizardAnnotation to a sample.
    This annotation is used later in anaphora resolver & wizard feature extractor.

    `WIZARD_ENTITY_FINDER_THRESHOLD` controls how conservative entity finder rule will be.
    * For greater values, the entities list gets smaller (entities with high confidence are returned).
    * For lower values, the entities list gets larger.
    Note: It's actually threshold on sigmoid logit. Default value is 0 -- means that entities with confidence >= 1/2
    are returned. We want to get as many entities as possible. Value `sigmoid(-2) = 0.11` which means
    that ~90% of entities will be returned.
    """

    WIZARD_ENTITY_FINDER_THRESHOLD = 0
    REQUESTED_RULES = [
        'AliceNormalizer',
        'AliceSession',
        'AliceTypeParserTime',
        'CustomEntities',
        'Date',
        'DirtyLang',
        'EntityFinder',
        'EntitySearch',
        'ExternalMarkup',
        'Fio',
        'GeoAddr',
        'Granet',
        'IsNav',
        'MusicFeatures',
        'Wares',
    ]
    # Rules which result is stored in the annotations
    STORED_RULES = REQUESTED_RULES + [
        'AliceAnaphoraSubstitutor'  # AliceAnaphoraSubstitutor result comes from MM request to wizard
    ]

    def __init__(self, wizard_url=None, **kwargs):
        super(WizardSampleProcessor, self).__init__(**kwargs)

        self._wizard = WizardHTTPAPI(url=wizard_url)

    @property
    def is_normalizing(self):
        return False

    def _process(self, sample, session, *args, **kwargs):
        request_id = kwargs.get('request_id')
        features = kwargs.get('features')
        not_use_wizard_from_mm_flag = kwargs.get('not_use_wizard_from_mm_flag')
        try:
            wizextra = 'entity_finder_threshold=%d' % self.WIZARD_ENTITY_FINDER_THRESHOLD
            if sample.partially_normalized_text:
                # To pass to granet the original query
                wizextra = [wizextra, 'alice_original_text=%s' % sample.partially_normalized_text]

            extra_params = {
                'wizextra': wizextra,
                'reqid': request_id,
                'rwr': ','.join(self.REQUESTED_RULES)
            }
            wizard_response = None
            if not_use_wizard_from_mm_flag is None and features is not None and isinstance(features, dict):
                wizard_response = features.get('wizard', None)
            if wizard_response is None:
                wizard_response = self._wizard.get_response(
                    text=' '.join(sample.tokens),
                    extra_params=extra_params,
                    request_id=request_id
                )
            wizard_rules = wizard_response.get('rules', {})
            markup = wizard_response.get('markup', {})
            token_alignment = self.align_tokens(markup, sample.tokens)
            rules = {rule_name: wizard_rules.get(rule_name, {}) for rule_name in self.STORED_RULES}

            sample.annotations['wizard'] = WizardAnnotation(markup=markup, rules=rules, token_alignment=token_alignment)
        except (RequestException, ValueError) as e:
            logger.warning('Can\'t extract wizard annotation on "%s". Reason: %s', sample.text, e.message)

        return sample

    @classmethod
    def align_tokens(cls, markup, sample_tokens):
        wizard_tokens = map(operator.itemgetter('Text'), markup.get('Tokens', []))
        wizard_delims = [d.get('Text', '') for d in markup.get('Delimiters', [])]

        token_to_indices = defaultdict(list)
        for i, t in enumerate(sample_tokens):
            token_to_indices[t].append(i)

        def sample_token_index(token, expected_index=0):
            candidates = filter(lambda x: x >= expected_index, token_to_indices[token])
            if len(candidates) == 0:
                return -1
            return min(candidates)

        buf = {}
        next_expected_index = 0

        def free_buf():
            buf['token'] = ''
            buf['idx'] = []

        def update_alignment(buffer, expected_index):
            token_index = sample_token_index(buffer['token'], expected_index=expected_index)
            for i in buffer['idx']:
                out[i] = token_index
            return token_index + 1  # we expect the next index to be close to the current index + 1

        free_buf()

        out = [-1] * len(wizard_tokens)

        # normalizer inserts delimiter symbols at the begin and at the end
        if not len(wizard_delims) == 1 + len(wizard_tokens):
            # if it is not the case, refuse to align
            return out
        if not wizard_delims[0].endswith(' '):
            buf['token'] = wizard_delims[0].strip()
        wizard_delims = wizard_delims[1:]

        for w_idx, (w_token, w_delim) in enumerate(zip(wizard_tokens, wizard_delims)):
            buf['token'] += w_token
            buf['idx'].append(w_idx)
            if ' ' in w_delim:
                # if whitespace delimiter met, assign one index to corresponding wizard tokens
                next_expected_index = update_alignment(buf, next_expected_index)
                free_buf()
                if not w_delim.endswith(' '):
                    # if w_delim contains a character after space, it should go into the next token
                    buf['token'] += w_delim.strip()
            else:
                # if other delimiter is found, than append tokens until whitespace
                buf['token'] += w_delim
        if buf['token']:
            update_alignment(buf, next_expected_index)
        return out
