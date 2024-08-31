# coding: utf-8
from __future__ import unicode_literals

import logging
import re
from itertools import izip

from nlu_service.entities.sample import Sample
from nlu_service.services.normalizer import reverse_normalize

# copy-pasted from vins


logger = logging.getLogger(__name__)


# only alphabetic
_ALPHA = '[^\W\d_]+'
# alphabetic + numbers
_ALPHANUM = '[^\W_]+'
# multi-words with separator (multi-word, multi/word, etc.)
# partial words contain only alphabetic symbols (any 1-word or word-1 will be ignored)
_COMPOSED_WORD = r'{0}(?:[\-/\\]{0})+'.format(_ALPHA)
# any number (int, float)
_NUMBER = '(?:\d+[,\.]\d+|\d+)'
# negative number -1 only at the start of the token
_SIGNED_NUMBER = '^\-{0}'.format(_NUMBER)
# datetime of type dd.mm.yy{yy}
_DATE = ur'\d{1,2}[\.,\\/\-]\d{1,2}[\.,\\/\-]\d{2,4}'
# time of type hh:mm
_TIME = '\d{1,2}:\d\d'
# arithmetic operations, if inside tokens, should be surrounded by digits
_ARITHMETIC = '(?:^|(?<=\d))[\+\-/\*=\^](?:$|(?=\d))'
# currency symbols
_CURRENCY = '[\$\u20ac\u20bd]'
# special cases with glued punctuation
_GLUED_PUNCT = ur'\w\+\+'


class FstNormalizerError(Exception):
    pass


class NormalizerEmptyResultError(Exception):
    pass


def group_by_slot(tokens, tags):
    """ Split sentence into same-slot sequences BI+ (normal slots) or I+ (continued slots) and iterate over them """
    slot_toks, slot_idx, slot_tags = [], [], []
    prev_tag = None
    for idx, (token, raw_tag) in enumerate(zip(tokens, tags)):
        tag = re.sub('^(B-|I-)', '', raw_tag)
        if tag != prev_tag or raw_tag.startswith('B-'):
            if prev_tag is not None:
                yield slot_toks, slot_idx, slot_tags, prev_tag
                slot_toks, slot_idx, slot_tags = [], [], []
        slot_toks.append(token)
        slot_idx.append(idx)
        slot_tags.append(raw_tag)
        prev_tag = tag
    if prev_tag is not None:
        yield slot_toks, slot_idx, slot_tags, prev_tag


class BaseSampleProcessor(object):
    """Abstract class for sample processors.
    Childs serve to DialogManager for processing sample inside DialogManager.handle(utterance, session)
    method.

    All childs reserve key names (attribute ``NAME``) when are registered in ``registry.py``.

    They can use session object for implementing complex logic.
    """

    NAME = None

    def __init__(self, **kwargs):
        pass

    @property
    def is_normalizing(self):
        """Should be True if processor changes text property of the Sample"""
        raise NotImplementedError

    def _process(self, sample, session, is_inference):
        """Only one method which must be implemented in each child class. Process sample and return a modified one.

        Parameters
        ----------
        sample : vins_core.common.sample.Sample
            Sample object.
        session : vins_core.dm.session.Session
            Session object.
        is_inference : bool
            Flag indicating whether it's an inference time or training/initialization phase.

        Returns
        -------
        new_sample : vins_core.common.sample.Sample
            Modified sample.
        """
        raise NotImplementedError

    def __call__(self, sample, session=None, is_inference=True):
        """Process sample and return a modified one.

        Parameters
        ----------
        sample : vins_core.common.sample.Sample
            Sample object.
        session : vins_core.dm.session.Session
            Session object.
        is_inference : bool, optional
            Flag indicating whether it's an inference time or training/initialization phase. Default is True.

        Returns
        -------
        new_sample : vins_core.common.sample.Sample
            Modified sample.
        """
        if sample:
            return self._process(sample, session, is_inference)
        else:
            return sample


class NormalizeSampleProcessor(BaseSampleProcessor):
    """Processor which normalizes input sample. Session need not to be provided in ``__call__`` method.
    """
    _TOKEN = ur'({0})'.format('|'.join((
        _GLUED_PUNCT, _COMPOSED_WORD, _DATE, _TIME, _SIGNED_NUMBER,
        _ARITHMETIC, _NUMBER, _ALPHANUM, _CURRENCY,
    )))

    def __init__(self, normalizer=None):
        super(NormalizeSampleProcessor, self).__init__()
        if not normalizer:
            raise ValueError('Normalizer is not specified')
        self._normalizer_name = normalizer
        self._normalizer = reverse_normalize  # normalizer_factory.get_normalizer(self._normalizer_name)

    @property
    def is_normalizing(self):
        return True

    @classmethod
    def _extract_tokens(cls, raw_token):
        return re.findall(cls._TOKEN, raw_token, flags=re.U)

    def _pre_normalization(self, sample):
        n_toks, n_tags = [], []

        for tok, tag in izip(sample.tokens, sample.tags):
            # normalizer bug: replace 'ε' symbol (special symbol in OpenFST)
            # check test_fakes_101
            # separating words glued together by comma
            toks = self._extract_tokens(tok)
            # remove singleton punctuation
            if not toks:
                continue
            n_toks.extend(toks)
            following_tag = tag
            if following_tag.startswith('B-'):
                following_tag = 'I-' + following_tag[2:]
            n_tags.extend([tag] + [following_tag] * (len(toks) - 1))
        return Sample.from_string(''.join(n_toks), tokens=n_toks, tags=n_tags)

    def _run_fst_normalizer(self, text):
            n_text = self._normalizer(text).lower().strip()
            # fix normalizer pattern:
            # leading zeros become space-delimited after denormalization
            zeros = re.search(u'((?: |^)0(?: 0)+)', n_text, re.UNICODE)
            if zeros:
                zstr = zeros.group(1)
                n_text = n_text.replace(zstr, ' ' + re.sub(u'\s+', u'', zstr))
            return n_text

    def _normalization(self, sample):
        if not self._normalizer:
            return sample
        out_toks, out_tags = [], []
        for toks, indices, tags, segment_tag in group_by_slot(sample.tokens, sample.tags):
            n_toks = self._run_fst_normalizer(' '.join(toks)).split()
            n_tags = [segment_tag] * len(n_toks)
            if segment_tag != 'O':
                # sequence of tags usually starts with B-tag, but can start with I-, if it is a continued slot
                first_prefix = tags[0][:2]
                n_tags[0] = first_prefix + n_tags[0]
                for i in xrange(1, len(n_tags)):
                    n_tags[i] = 'I-' + n_tags[i]
            out_toks.extend(n_toks)
            out_tags.extend(n_tags)

        self._validate_normalization(sample.tokens, out_toks)

        return Sample.from_string(''.join(out_toks), tokens=out_toks, tags=out_tags)

    def _validate_normalization(self, input_tokens, output_tokens):
        normalized_tokens = self._run_fst_normalizer(' '.join(input_tokens)).split()
        if normalized_tokens != output_tokens:
            raise FstNormalizerError(
                'Tokens might be unaligned with tags for sample:\n'
                'Fully normalized utterance: "%s"\n'
                'Partially normalized utterance: "%s"\n'
                'Initial input: "%s"',
                ' '.join(normalized_tokens),
                ' '.join(output_tokens),
                ' '.join(input_tokens)
            )

    @classmethod
    def _post_normalization(cls, sample):
        n_toks = []
        for tok in sample.tokens:
            # remove redundant whitespaces if any
            tok = re.sub(u'\s+', ' ', tok)
            # ё normalization
            tok = re.sub(u'ё', u'е', tok, re.UNICODE | re.IGNORECASE)
            n_toks.append(tok)
        return Sample.from_string(''.join(n_toks), tokens=n_toks, tags=sample.tags)

    def _process(self, sample, *args, **kwargs):
        text = sample.text
        utterance = sample.utterance
        weight = sample.weight
        app_id = sample.app_id
        sample = self._pre_normalization(sample)
        sample = self._normalization(sample)
        sample = self._post_normalization(sample)
        if len(sample) == 0:
            raise NormalizerEmptyResultError('Normalizer returns empty result for input "%s"' % text)
        return Sample(
            utterance=utterance,
            tokens=sample.tokens,
            tags=sample.tags,
            weight=weight,
            app_id=app_id
        )
